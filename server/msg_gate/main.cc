#include <gflags/gflags.h>
#include <spdlog/spdlog.h>
#include <grpcpp/server_builder.h>
#include <memory>
#include <thread>
#include "ws_app.h"
#include "user_mgr.h"
#include "msg_service_impl.h"

DEFINE_string(rpc_address, "0.0.0.0:15000", "smtp session最大数量");

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  spdlog::set_level(spdlog::level::debug);

  // 单例初始化
  msg::msg_gate::UserManager::Instance();
  // 启动grpc server
  msg::msg_gate::MsgServiceImpl service;
  grpc::ServerBuilder builder;
  builder.AddListeningPort(FLAGS_rpc_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  spdlog::info("服务启动，地址：{}", FLAGS_rpc_address);
  // 简单的线程池
  std::vector<std::unique_ptr<std::thread>> threads(std::thread::hardware_concurrency());
  // 启动ws服务
  msg::msg_gate::RunWsApp(&threads);
  // grpc等待线程
  threads.emplace_back(std::make_unique<std::thread>([&server](){ server->Wait(); }));
  // join
  std::for_each(threads.begin(), threads.end(), [](std::unique_ptr<std::thread>& t) {
    t->join();
  });
}