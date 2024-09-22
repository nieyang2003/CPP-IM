// grpc service实现，处理push

#pragma once
#include <grpcpp/server_builder.h>
#include "user_mgr.h"
#include "include/thread_pool.h"
#include "proto/push.grpc.pb.h"
#include "proto/push.pb.h"

namespace msg::msg_gate {

/// @brief gate server，
class GateServer : public Singleton<GateServer> {
  friend class Singleton<GateServer>;
 public:
  /// @brief 析构，关闭完成队列和服务
  ~GateServer();

  // 运行grpc服务，接受请求放入线程池异步处理
  void Run(const std::string& location, std::shared_ptr<ThreadPool> pool);

 protected:
  GateServer() = default;

 private:
  // 当前服务器的完成队列，处理异步请求的生命周期
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  // 当前服务器的异步服务，处理 gRPC 服务的异步调用
  push::GateService::AsyncService service_;
  // 服务器实例
  std::unique_ptr<grpc::Server> server_;
};

} // namespace msg