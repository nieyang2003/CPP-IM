#pragma once
#include <librdkafka/rdkafkacpp.h>
#include "include/singleton.h"
#include "include/thread_pool.h"

namespace transfer {

class Consumer : public Singleton<Consumer> {
  friend class Singleton<Consumer>;
 public:
  typedef void(*Handler)(std::shared_ptr<RdKafka::Message> message);

  Consumer();

  bool Init(const std::string& groupid, const std::string& topic);

  void Run();

  void Consume(std::shared_ptr<RdKafka::Message> message);

  void SetHandler(Handler h) { handler_ = h; }

  ThreadPool& GetThreadPool() { return thread_pool_; }

 private:
  class EventCb : public RdKafka::EventCb {
   public:
    void event_cb (RdKafka::Event &event) override;
  };

  class RebalanceCb : public RdKafka::RebalanceCb {
   public:
    void rebalance_cb (RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,
                       std::vector<RdKafka::TopicPartition*>&partitions) override;
  };

 private:
  bool stop_ = false;
  EventCb event_cb_;
  RebalanceCb rb_cb_;
  std::unique_ptr<RdKafka::Conf> conf_ = nullptr; // 全局配置对象，用于配置生产者的全局参数，如 Kafka 代理地址
  std::unique_ptr<RdKafka::Conf> tconf_ = nullptr; // 主题配置对象，用于配置与主题相关的参数
  std::unique_ptr<RdKafka::KafkaConsumer > consumer_ = nullptr; // Kafka 消费者实例
  Handler handler_ = nullptr;
  ThreadPool thread_pool_;
};

}