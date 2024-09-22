#include "route_service_impl.h"
#include <gflags/gflags.h>
#include <grpcpp/server_builder.h>
#include <spdlog/spdlog.h>
#include "include/redis_pool.h"
#include "include/spd_gflag.h"

DEFINE_string(service_address, "0.0.0.0:10003", "smtp session最大数量");
DEFINE_string(redis_host, "127.0.0.1", "redis host");
DEFINE_int32(log_level, 0, "日志等级");

int main(int argc, char** argv) {
  // 读取选项配置
  InitSpdGflag(argc, argv, "route");
  // redis连接池
  redis::Init(FLAGS_redis_host);
  // 创建grpc服务
  route::RouteServiceImpl service;
  grpc::ServerBuilder builder;
  builder.AddListeningPort(FLAGS_service_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  // 启动
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  spdlog::info("服务启动，地址：{}", FLAGS_service_address);
  server->Wait();
  return 0;
}