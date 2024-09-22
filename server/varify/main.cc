#include <gflags/gflags.h>
#include <grpcpp/server_builder.h>
#include <spdlog/spdlog.h>
#include "varify_service_impl.h"
#include "smtp.h"
#include "redis_pool.h"

DEFINE_string(service_address, "0.0.0.0:10004", "smtp session最大数量");

int main(int argc, char** argv) {
  // 读取选项配置
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  spdlog::set_level(spdlog::level::debug);
  // 初始化单例
  varify::SmtpPoll::Instance();
  varify::RedisPool::Instance();
  // 创建grpc服务
  varify::VarifyServiceImpl service;
  grpc::ServerBuilder builder;
  builder.AddListeningPort(FLAGS_service_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  // 启动
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  spdlog::info("服务启动，地址：{}", FLAGS_service_address);
  server->Wait();
  return 0;
}