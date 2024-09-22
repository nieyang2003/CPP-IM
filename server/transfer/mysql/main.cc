#include <Poco/Data/SessionPool.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>
#include <Poco/Data/MySQL/Connector.h>
#include <Poco/Data/DataException.h>
#include <Poco/Data/RecordSet.h>
#include <gflags/gflags.h>
#include <spdlog/spdlog.h>
#include <librdkafka/rdkafkacpp.h>
#include <memory>
#include "transfer/kafka_consumer.h"
#include "proto/msg.pb.h"
#include "include/thread_pool.h"
#include "include/mysql_pool.h"

DEFINE_string(mysql_conn, "host=127.0.0.1;user=root;password=123456;db=ychat", "MySQL连接配置");

DEFINE_string(groupid, "mysql", "");
DEFINE_string(topic, "ychat", "");

void MySQLHandle(std::shared_ptr<RdKafka::Message> message) {
  uint64_t from_uid = 0;
  uint64_t to_id = 0;
  int64_t sendtime = 0;
  std::string msgid;
  bool is_send_to_group = false;

  {
    msg::MqMessage mq_message;
    if (!mq_message.ParseFromArray(static_cast<const char*>(message->payload()), message->len())) {
      spdlog::error("反序列化失败");
      return;
    }
    if (mq_message.handle_result().msgid() != *message->key()) {
      spdlog::error("key错误, `{}` != `{}`", mq_message.handle_result().msgid(), *message->key());
      return;
    }

    from_uid = mq_message.origin_req().from_uid();
    to_id = mq_message.origin_req().to_id();
    sendtime = mq_message.handle_result().timestamp();
    msgid = mq_message.handle_result().msgid();
    // 是否是群消息
    if (mq_message.origin_req().type() == msg::ProtoReqType::TypeGroupMsg ||
        mq_message.origin_req().type() == msg::ProtoReqType::TypeGroupCtrl) {
      is_send_to_group = true;
    }
  }

  try {
    {
      // 写入消息表
      std::vector<uint8_t> data((const uint8_t*)message->payload(), ((const uint8_t*)message->payload()) + message->len());
      mysql::CreateStatement() << 
        "INSERT INTO messages (msgid, data) VALUES (?, ?)",
        Poco::Data::Keywords::use(msgid),
        Poco::Data::Keywords::use(data),
        Poco::Data::Keywords::now;
      // 清除数据data和message
      message.reset();
      spdlog::info("写入消息表");
    }
    if (is_send_to_group) {
	  // 写入群收件箱
	  mysql::CreateStatement() <<
        "INSERT INTO group_box (gid, sendtime, msgid) VALUES (?, ?, ?)",
	    Poco::Data::Keywords::use(to_id),
		Poco::Data::Keywords::use(sendtime),
		Poco::Data::Keywords::use(msgid),
        Poco::Data::Keywords::now;
      spdlog::info("群收件箱");
    } else {
	  // 写入用户收件箱
	  mysql::CreateStatement() << 
        "INSERT INTO user_box (from_uid, to_uid, sendtime, msgid) VALUES (?, ?, ?, ?)",
        Poco::Data::Keywords::use(from_uid),
	    Poco::Data::Keywords::use(to_id),
		Poco::Data::Keywords::use(sendtime),
		Poco::Data::Keywords::use(msgid),
        Poco::Data::Keywords::now;
      spdlog::info("写入用户收件箱");
	}
  } catch (const Poco::Data::DataException& e) {
    spdlog::error("数据库操作失败: {}", e.what());
  }

  return;
}

int main(int argc, char** argv) {
  mysql::InitMysql(FLAGS_mysql_conn);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  spdlog::set_level(spdlog::level::debug);

  if (!transfer::Consumer::Instance()->Init(FLAGS_groupid, FLAGS_topic)) {
	spdlog::error("init kafka consumer error");
	return 1;
  }
  transfer::Consumer::Instance()->SetHandler(MySQLHandle);
  transfer::Consumer::Instance()->Run();
  return 0;
}