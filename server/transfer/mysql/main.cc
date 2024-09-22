#include <Poco/Data/SessionPool.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>
#include <Poco/Data/MySQL/Connector.h>
#include <Poco/Data/MySQL/MySQLException.h>
#include <Poco/Data/RecordSet.h>
#include <gflags/gflags.h>
#include <spdlog/spdlog.h>
#include <librdkafka/rdkafkacpp.h>
#include <memory>
#include "transfer/kafka_consumer.h"

DEFINE_string(host, "", "");
DEFINE_string(user, "", "");
DEFINE_string(pass, "", "");
DEFINE_string(db, "", "");
DEFINE_uint32(min_conns, 4, "");
DEFINE_uint32(max_conns, 16, "");
DEFINE_uint32(max_alive, 180, "");
DEFINE_string(groupid, "groupid", "");
DEFINE_string(topic, "topic", "");

std::unique_ptr<Poco::Data::SessionPool> pool;

bool InitMySQL() {
  // 注册 MySQL 连接器
  Poco::Data::MySQL::Connector::registerConnector();
  pool = std::make_unique<Poco::Data::SessionPool>("MySQL",
    fmt::format("host={};user={};password={};db={}", FLAGS_host, FLAGS_user, FLAGS_pass, FLAGS_db),
	FLAGS_min_conns,
	FLAGS_max_conns,
	FLAGS_max_alive);
  try {
    Poco::Data::Session session(pool->get());
	Poco::Data::Statement statement(session);
    statement << "SHOW TABLES";
	statement.execute();
	spdlog::info("initing mysql, try exec `SHOW TABLES;`");
	// 获取结果集
    Poco::Data::RecordSet rs(statement);
	std::size_t count = rs.rowCount();
	spdlog::info("init mysql success, {} tables in {}", count, FLAGS_db);
	// 打印
	for (std::size_t i = 0; i < count; ++i) {
	  spdlog::debug("{}", rs[i][0].convert<std::string>());  // 获取每一行的第一列数据
    }
	return true;
  } catch (Poco::Data::MySQL::MySQLException& ex) {
    spdlog::error("MySQL Exception: {}", ex.displayText());
  } catch (Poco::Exception& ex) {
	spdlog::error("Poco Exception: {}", ex.displayText());
  }
  return false;
}

void FiniMySQL() {
  spdlog::info("fini mysql");
  Poco::Data::MySQL::Connector::unregisterConnector();
}

void MySQLHandle(std::shared_ptr<RdKafka::Message> message) {
  // 写入mysql
}

int main() {
  std::shared_ptr<int> defer((int*)!!InitMySQL(), [](int*){
	FiniMySQL();
  });
  if (!transfer::Consumer::Instance()->Init(FLAGS_groupid, FLAGS_topic)) {
	spdlog::error("init kafka consumer error");
	return 1;
  }
  transfer::Consumer::Instance()->Run();
}