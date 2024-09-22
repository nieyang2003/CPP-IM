#pragma once
#include <grpcpp/server_builder.h>
#include <sw/redis++/redis++.h>
#include "include/thread_pool.h"
#include "proto/push.pb.h"
#include "proto/push.grpc.pb.h"
#include "async_gate_client.h"

namespace push {

struct PushContext {
  // server
  bool done = false;
  grpc::ServerContext ctx;                            // rpc服务的上下文信息
  msg::PushReq req;                                   //请求数据类型
  msg::PushResp resp;                                    //响应数据类型
  grpc::ServerAsyncResponseWriter<msg::PushResp> responder; //响应器
  // client
  grpc::Status status;
  grpc::ClientContext client_context;
  std::unique_ptr<grpc::ClientAsyncResponseReader<msg::PushResp>> resp_reader;
  // 构造方法
  PushContext() : responder(&ctx) {}
};

class PushServer {
 public:
  PushServer(const std::string& location, size_t thread_num);
  ~PushServer();
  void Run();

 private:
  // 当前服务器的地址
  std::string server_locaton_;
  // 当前服务器的完成队列，处理异步请求的生命周期
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  // 当前服务器的异步服务，处理 gRPC 服务的异步调用
  PushService::AsyncService service_;
  // 服务器实例
  std::unique_ptr<grpc::Server> server_;
  // 线程池
  ThreadPool thread_pool_;
  // 网关服务
  GateClient gate_;
};

}