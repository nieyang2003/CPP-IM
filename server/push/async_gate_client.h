#pragma once
#include <gflags/gflags.h>
#include <grpcpp/grpcpp.h> 
#include <thread>
#include "proto/push.grpc.pb.h"

namespace push {

class PushContext;

class GateClient {
 public:
  void Push(PushContext* context, const std::string& gate_address);
  GateClient();
  ~GateClient();

  void Run();

 private:
  void AsyncRpcComplete();

 private:
  std::mutex stub_lock_;
  std::map<std::string, std::unique_ptr<GateService::Stub>> stubs_;
  // 异步任务
  std::unique_ptr<std::thread> async_rpc_complete_thread_;
  grpc::CompletionQueue cq_;
};

} // namespace push
