#include "kafka_producer.h"
#include <gflags/gflags.h>
#include <spdlog/spdlog.h>

DEFINE_string(kafka_brokers, "127.0.0.1:9092", "Kafka broker 地址");
DEFINE_string(topic_outbox, "127.0.0.1:9092", "用户发件箱");
DEFINE_string(topic_inbox, "127.0.0.1:9092", "用户收件箱");

// TODO: 发件箱topic

namespace logic {

/// @brief kafka消息交付报告回调
class ReportCb : public RdKafka::DeliveryReportCb {
 public:
  void dr_cb (RdKafka::Message &message) override {
	if (message.err()) [[unlikely]] {
      // TODO:
	  spdlog::error("交付失败");
	} else {
	  // TODO:
	  spdlog::info("交付成功");
	}
  }
};

bool Producer::Init() {
  // 创建配置对象
  conf_.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
  tconf_.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC));
  // 设置 Kafka broker 地址并将参数应用到配置对象
  std::string errstr;
  if (conf_->set("bootstrap.servers", FLAGS_kafka_brokers, errstr) != RdKafka::Conf::CONF_OK) {
	spdlog::error("Failed to set brokers: {}", errstr);
    return false;
  }
  // 设置交付回调
  ReportCb report_cb;
  if (conf_->set("dr_cb", &report_cb, errstr) != RdKafka::Conf::CONF_OK) {
	spdlog::error("Failed to set delivery report callback: {}", errstr);
    return false;
  }
  // 创建 Kafka 生产者实例
  producer_.reset(RdKafka::Producer::create(conf_.get(), errstr));
  if (!producer_) {
	  spdlog::error("Failed to create producer: {}", errstr);
      return false;
  }
  // 收件箱topic
  inbox_topic_handle_.reset(RdKafka::Topic::create(producer_.get(), FLAGS_topic_inbox, tconf_.get(), errstr));
  if (!inbox_topic_handle_) {
	  spdlog::error("Failed to create topic: {}", errstr);
      return false;
  }
  return true;
}

bool Producer::ProduceToInbox(std::shared_ptr<msg::PushMsg> msg) {
  std::string payload;
  msg->SerializeToString(&payload);  // 序列化为二进制数据

  RdKafka::ErrorCode resp = producer_->produce(
    inbox_topic_handle_.get(),
    RdKafka::Topic::PARTITION_UA,
    RdKafka::Producer::RK_MSG_COPY /* 可以替换为 RK_MSG_FREE 以减少内存拷贝 */,
    const_cast<char*>(payload.data()), payload.size(),
    nullptr, nullptr);

  if (resp != RdKafka::ERR_NO_ERROR) {
    spdlog::error("Failed to produce message: {}", RdKafka::err2str(resp));
    return false;
  }

  // 检查并处理生产者的事件，立即返回
  producer_->poll(0);
  return true;
}

} // namespace logic