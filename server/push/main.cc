#include <gflags/gflags.h>
#include <spdlog/spdlog.h>
#include "push_server.h"
#include "include/redis_pool.h"
#include "include/spd_gflag.h"

DEFINE_string(service_address, "127.0.0.1:10002", "grpc服务启动地址");
DEFINE_string(redis_host, "127.0.0.1", "grpc服务启动地址");
DEFINE_int32(log_level, 0, "日志等级");

int main(int argc, char** argv) {
  InitSpdGflag(argc, argv, "push");
  redis::Init(FLAGS_redis_host);

  push::PushServer server(FLAGS_service_address, std::thread::hardware_concurrency());
  server.Run();

  return 0;
}