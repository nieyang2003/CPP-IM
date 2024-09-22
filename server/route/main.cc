#include "route_service_impl.h"
#include <gflags/gflags.h>
#include <grpcpp/server_builder.h>
#include <spdlog/spdlog.h>

DEFINE_string(server_address, "0.0.0.0:13000", "smtp session最大数量");

int main(int argc, char** argv) {
  // 读取选项配置
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  spdlog::set_level(spdlog::level::debug);
  // 初始化单例
  // 创建grpc服务
  route::RouteServiceImpl service;
  grpc::ServerBuilder builder;
  builder.AddListeningPort(FLAGS_server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  // 启动
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  spdlog::info("服务启动，地址：{}", FLAGS_server_address);
  server->Wait();
  return 0;
  return 0;
}