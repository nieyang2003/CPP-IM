#include <gflags/gflags.h>
#include <spdlog/spdlog.h>
#include <librdkafka/rdkafkacpp.h>
#include <memory>
#include "transfer/kafka_consumer.h"

DEFINE_string(mongo_location, "localhost:27017", "");
DEFINE_uint32(mongo_min_conns, 4, "");
DEFINE_uint32(mongo_max_conns, 16, "");
DEFINE_string(groupid, "groupid", "");
DEFINE_string(topic, "topic", "");

void MongonHandle(std::shared_ptr<RdKafka::Message> message) {
  // 是否在线，在线不写直接推送
}

int main(int argc, char** argv) {

	return 0;
}