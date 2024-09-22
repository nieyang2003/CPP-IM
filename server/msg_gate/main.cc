#include <grpcpp/server_builder.h>
#include <memory>
#include <thread>
#include <sw/redis++/redis++.h>
#include "include/thread_pool.h"
#include "include/redis_pool.h"
#include "include/spd_gflag.h"
#include "ws_app.h"
#include "user_mgr.h"
#include "async_logic_client.h"
#include "gate_server.h"

DEFINE_int32(log_level, 0, "日志等级");
DEFINE_string(service_address, "127.0.0.1:10000", "");
DEFINE_string(redis_host, "127.0.0.1", "redis host");

int main(int argc, char** argv) {
  // 初始化配置和日志
  InitSpdGflag(argc, argv, "gate");
  // 单例
  msg::msg_gate::UserManager::Instance();
  msg::msg_gate::LogicClient::Instance();
  msg::msg_gate::GateServer::Instance();
  // 线程池
  auto pool = std::make_shared<ThreadPool>();
  // redis连接池
  redis::Init(FLAGS_redis_host);
  // ws_app
  pool->Enqueue([&](){ msg::msg_gate::RunWsApp(pool, FLAGS_service_address); }); // 占用一个线程
  // gate_server
  msg::msg_gate::GateServer::Instance()->Run(FLAGS_service_address, pool); // 阻塞

  return 0;
}