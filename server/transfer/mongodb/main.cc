#include <gflags/gflags.h>
#include <spdlog/spdlog.h>
#include <librdkafka/rdkafkacpp.h>
#include <memory>
#include <mongocxx/client.hpp>
#include <mongocxx/pool.hpp>
#include "transfer/kafka_consumer.h"

DEFINE_string(mongo_location, "localhost:27017", "");
DEFINE_uint32(mongo_min_conns, 4, "");
DEFINE_uint32(mongo_max_conns, 16, "");
DEFINE_string(groupid, "mongodb", "");
DEFINE_string(topic, "ychat", "");

std::unique_ptr<mongocxx::pool> mongo_pool;

void MongonHandle(std::shared_ptr<RdKafka::Message> message) {
  // 是否在线，在线不写直接推送
  auto client = mongo_pool->acquire();
  spdlog::debug("test");
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  spdlog::set_level(spdlog::level::debug);

  auto uri_str = fmt::format("mongodb://{}/?minPoolSize={}&maxPoolSize={}", FLAGS_mongo_location, FLAGS_mongo_min_conns, FLAGS_mongo_max_conns);
  mongocxx::uri uri{uri_str};
  mongo_pool = std::make_unique<mongocxx::pool>(uri);
  if (!mongo_pool) {
	spdlog::error("init mongodb pool error");
	return 1;
  }

  if (!transfer::Consumer::Instance()->Init(FLAGS_groupid, FLAGS_topic)) {
	spdlog::error("init kafka consumer error");
	return 1;
  }
  transfer::Consumer::Instance()->SetHandler(&MongonHandle);
  transfer::Consumer::Instance()->Run();
}