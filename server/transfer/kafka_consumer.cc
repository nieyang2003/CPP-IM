#include "kafka_consumer.h"
#include <spdlog/spdlog.h>
#include <gflags/gflags.h>
#include <csignal>
#include "../include/thread_pool.h"

namespace transfer {

namespace {

DEFINE_string(kafka_brokers, "127.0.0.1:9092", "Kafka broker 地址");

} // namespace

Consumer::Consumer()
  : thread_pool_(std::thread::hardware_concurrency()) {
}

bool Consumer::Init(const std::string &groupid, const std::string &topic) {
  // 创建配置对象
  conf_.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
  tconf_.reset(RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC));
  // 设置 Kafka broker 地址并将参数应用到配置对象
  std::string errstr;
  if (conf_->set("bootstrap.servers", FLAGS_kafka_brokers, errstr) != RdKafka::Conf::CONF_OK) {
  	spdlog::error("Failed to set brokers: {}", errstr);
  	return false;
  }
  // 设置 Kafka broker 地址并将参数应用到配置对象
  if (conf_->set("group.id", groupid, errstr) != RdKafka::Conf::CONF_OK) {
  	spdlog::error("Failed to set group: {}", errstr);
  	return false;
  }
  // 设置 Kafka broker 地址并将参数应用到配置对象
  if (conf_->set("enable.partition.eof", "true", errstr) != RdKafka::Conf::CONF_OK) {
  	spdlog::error("Failed to set enable.partition.eof: {}", errstr);
  	return false;
  }
  // Set up rebalance callback
  if (conf_->set("event_cb", &event_cb_, errstr) != RdKafka::Conf::CONF_OK) {
  	spdlog::error("Failed to set event_cb: {}", errstr);
  	return false;
  }
  if (conf_->set("rebalance_cb", &rb_cb_, errstr) != RdKafka::Conf::CONF_OK) {
  	spdlog::error("Failed to set event_cb: {}", errstr);
  	return false;
  }
  // 创建生产者
  consumer_.reset(RdKafka::KafkaConsumer::create(conf_.get(), errstr));
  if (!consumer_) {
  	spdlog::error("Failed to create consumer: {}", errstr);
  	return false;
  }
  // 订阅topic
  RdKafka::ErrorCode err = consumer_->subscribe({topic});
  if (err) {
  	spdlog::error("Failed to subscribe to {}: {}", topic, RdKafka::err2str(err));
  	return false;
  }
  return true;
}

void Consumer::Run() {
  while (!stop_) [[likely]] {
	std::shared_ptr<RdKafka::Message> msg(consumer_->consume(1000));
	thread_pool_.Enqueue([msg, this](){
	  Consume(msg);
	});
  }
  consumer_->close();
  RdKafka::wait_destroyed(5000); // 等待所有 librdkafka 对象被销毁
}

void Consumer::Consume(std::shared_ptr<RdKafka::Message> message) {
  switch (message->err()) {
    case RdKafka::ERR__TIMED_OUT:
      break;
	case RdKafka::ERR_NO_ERROR: {
	  // 放入注册的函数中
	  handler_(message);
	  break;
	}
	case RdKafka::ERR__PARTITION_EOF: {
	  spdlog::info("Reached end of partition");
	  break;
	}
	case RdKafka::ERR__UNKNOWN_TOPIC:
    case RdKafka::ERR__UNKNOWN_PARTITION: {
	  spdlog::error("Consume failed: {}", message->errstr());
	  stop_ = true;
	  break;
	}
	default: {
	  spdlog::error("Consume failed: {}", message->errstr());
	  stop_ = true;
	  break;
	}
  }
}

void Consumer::EventCb::event_cb(RdKafka::Event &event){
  switch (event.type()) {
    case RdKafka::Event::EVENT_ERROR: {
  	  spdlog::error("EVENT_ERROR ({}): {}", RdKafka::err2str(event.err()), event.str());
  	  break;
    }
    case RdKafka::Event::EVENT_STATS: {
  	  spdlog::info("EVENT_STATS {}", event.str());
  	  break;
    }
    case RdKafka::Event::EVENT_LOG: {
  	  spdlog::info("EVENT_LOG-{}-{}: {}", event.severity(), event.fac(), event.str());
  	  break;
    }
    case RdKafka::Event::EVENT_THROTTLE: {
  	  spdlog::error("EVENT_THROTTLE: {} ms by {} id {}", event.throttle_time(), event.broker_name(), event.broker_id());
  	  break;
    }
    default: {
  	  spdlog::error("EVENT{} ({}): {}", event.type(),  RdKafka::err2str(event.err()), event.str());
    }
  }
}

void Consumer::RebalanceCb::rebalance_cb(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,
  std::vector<RdKafka::TopicPartition *> &partitions){
  if (err == RdKafka::ERR__ASSIGN_PARTITIONS) {
    spdlog::info("Assigning partitions...");
    consumer->assign(partitions);
  } else if (err == RdKafka::ERR__REVOKE_PARTITIONS) {
    spdlog::info("Revoking partitions...");
    consumer->unassign();  // 撤销分区分配
  } else {
    spdlog::error("Rebalance error: ", RdKafka::err2str(err));
  }
}

}