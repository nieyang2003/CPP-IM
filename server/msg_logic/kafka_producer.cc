#include "kafka_producer.h"
#include <gflags/gflags.h>
#include <spdlog/spdlog.h>
#include "logic_server.h"

DEFINE_string(kafka_brokers, "127.0.0.1:9092", "Kafka broker 地址");
DEFINE_string(kafka_topic, "ychat", "流水线");

namespace logic {

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
  if (conf_->set("dr_cb", &report_cb_, errstr) != RdKafka::Conf::CONF_OK) {
	spdlog::error("Failed to set delivery report callback: {}", errstr);
    return false;
  }
  // 设置消息重试次数
  if (conf_->set("retries", "3", errstr) != RdKafka::Conf::CONF_OK) {
    spdlog::error("Failed to set retries: {}", errstr);
    return false;
  }
  // 设置消息超时时间
  if (conf_->set("message.timeout.ms", "3000", errstr) != RdKafka::Conf::CONF_OK) {
    spdlog::error("Failed to set message timeout: {}", errstr);
    return false;
  }
  // 创建 Kafka 生产者实例
  producer_.reset(RdKafka::Producer::create(conf_.get(), errstr));
  if (!producer_) {
	  spdlog::error("Failed to create producer: {}", errstr);
      return false;
  }
  // 获取元数据来检查 Kafka 集群状态
  RdKafka::Metadata* metadata;
  RdKafka::ErrorCode err = producer_->metadata(true, nullptr, &metadata, 5000);
  if (err != RdKafka::ERR_NO_ERROR) {
    spdlog::error("Failed to fetch metadata: {}", RdKafka::err2str(err));
    return false;
  }
  // 收件箱topic
  topic_handle_.reset(RdKafka::Topic::create(producer_.get(), FLAGS_kafka_topic, tconf_.get(), errstr));
  if (!topic_handle_) {
	  spdlog::error("Failed to create topic: {}", errstr);
      return false;
  }
  return true;
}

bool Producer::Produce(std::shared_ptr<msg::MqMessage> msg) {
  spdlog::debug("写入kafka");
  std::string payload;
  msg->SerializeToString(&payload);  // 序列化为二进制数据

  RdKafka::ErrorCode resp = producer_->produce(
    topic_handle_.get(),
    RdKafka::Topic::PARTITION_UA,
    RdKafka::Producer::RK_MSG_COPY /* 可以替换为 RK_MSG_FREE 以减少内存拷贝 */,
    const_cast<char*>(payload.data()), payload.size(),
    &msg->handle_result().msgid(),
    nullptr);

  if (resp != RdKafka::ERR_NO_ERROR) {
    spdlog::error("Failed to produce message: {}", RdKafka::err2str(resp));
    return false;
  }

  // 检查并处理生产者的事件，立即返回
  producer_->poll(0);
//   producer_->poll(100);
  spdlog::info("已写入kafka发送队列");
  return true;
}

} // namespace logic