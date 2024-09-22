// 向消息队列中生产数据

#pragma once
#include <librdkafka/rdkafkacpp.h>
#include <memory>
#include <spdlog/spdlog.h>
#include "msg.pb.h"
#include  "include/singleton.h"

namespace logic {

/// @brief 
class Producer : public Singleton<Producer> {
  friend class Singleton<Producer>;
 public:
  bool Init();

  // 同步地生产消息到对方收件箱
  bool ProduceToInbox(std::shared_ptr<msg::PushMsg> msg);

 private:
  std::unique_ptr<RdKafka::Conf> conf_ = nullptr; // 全局配置对象，用于配置生产者的全局参数，如 Kafka 代理地址
  std::unique_ptr<RdKafka::Conf> tconf_ = nullptr; // 主题配置对象，用于配置与主题相关的参数
  std::unique_ptr<RdKafka::Producer> producer_ = nullptr; // Kafka 生产者实例
  std::unique_ptr<RdKafka::Topic> inbox_topic_handle_ = nullptr;
  std::unique_ptr<RdKafka::Topic> outbox_topic_handle_ = nullptr;
};

} // namespace logic