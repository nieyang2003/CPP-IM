#pragma once
#include <string>
#include <grpcpp/server_builder.h>
#include <spdlog/spdlog.h>
#include "msg.pb.h"
#include "msg.grpc.pb.h"
#include "include/thread_pool.h"

namespace logic {

class LogicServer {
  // 当前服务器的地址
  std::string server_locaton_;
  // 当前服务器的完成队列，处理异步请求的生命周期
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  // 当前服务器的异步服务，处理 gRPC 服务的异步调用
  msg::LogicService::AsyncService service_;
  // 服务器实例
  std::unique_ptr<grpc::Server> server_;

  /// @brief 存储每个 gRPC 调用的上下文信息
  /// @tparam RequestType 
  /// @tparam ResponseType 
  template <typename RequestType, typename ResponseType>
  struct RpcContext { // 继承base可以做多个函数
    bool done_;
	grpc::ServerContext ctx_;                           // rpc服务的上下文信息
    RequestType req_;                                   //请求数据类型
    ResponseType resp_;                                 //响应数据类型
    grpc::ServerAsyncResponseWriter<ResponseType> responder_; //响应器

    RpcContext() : done_(false), responder_(&ctx_) {}             //构造方法
  };
  
  /// @brief proto中定义的rpc函数的特化
  typedef RpcContext<msg::Protocol, msg::MsgInfo> MsgHandleRpcContext;

  // 业务处理函数
  grpc::Status Handle(grpc::ServerContext *context, const msg::Protocol &request, msg::MsgInfo *response);

 public:
  LogicServer(const std::string& location)
    : server_locaton_(location) {}
  ~LogicServer() { server_->Shutdown(); cq_->Shutdown(); }

  void Run(size_t thread_num = std::thread::hardware_concurrency());

};
	
}