#pragma once
#include <string>
#include <gflags/gflags.h>
#include <grpcpp/grpcpp.h> 
#include "include/singleton.h"
#include "include/connection_pool.h"
#include "proto/msg.pb.h"
#include "proto/msg.grpc.pb.h"

namespace msg::msg_gate {

class LogicStubs : public ConnectionPool<LogicService::Stub> {
 public:

  LogicStubs(const std::string& url, uint32_t max);
  std::unique_ptr<LogicService::Stub> CreateConnection() override;

 private:
  std::string url_;
};

class LogicClient : public Singleton<LogicClient> {
  friend class Singleton<LogicClient>;
 public:
  void SendToLogic(msg::ProtoReq&& proto, uint64_t uid);
  ~LogicClient();

 private:
  LogicClient();

 private:
  void AsyncRpcComplete();

 private:
  // 业务
  std::unique_ptr<LogicStubs> stubs_;

  // 异步任务
  std::unique_ptr<std::thread> async_rpc_complete_thread_;
  grpc::CompletionQueue cq_;
};

} // namespace msg::msg_gate