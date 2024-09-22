#include "include/spd_gflag.h"
#include "kafka_producer.h"
#include "msg_logic/logic_server.h"

DEFINE_string(service_address, "127.0.0.1:10001", "");
DEFINE_int32(log_level, 0, "日志等级");

int main(int argc, char** argv) {
  InitSpdGflag(argc, argv, "logic");

  // 单例
  if (!logic::Producer::Instance()->Init()) {
    spdlog::error("初始化Producer失败，退出...");
    return 1;
  }
  spdlog::info("初始化Producer成功");
  logic::LogicServer server(FLAGS_service_address, std::thread::hardware_concurrency());
  server.Run();
}