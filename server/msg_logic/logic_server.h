#pragma once
#include <string>
#include <grpcpp/server_builder.h>
#include <spdlog/spdlog.h>
#include <librdkafka/rdkafkacpp.h>
#include "proto/msg.pb.h"
#include "proto/msg.grpc.pb.h"
#include "include/thread_pool.h"

namespace logic {

class LogicServer {
 public:

  /// @brief 存储每个 gRPC 调用的上下文信息
  template <typename RequestType, typename ResponseType>
  struct RpcContext { // 继承base可以做多个函数
    bool done_ = false;
	grpc::ServerContext ctx_;                           // rpc服务的上下文信息
    RequestType req_;                                   //请求数据类型
    ResponseType resp_;                                 //响应数据类型
    grpc::ServerAsyncResponseWriter<ResponseType> responder_; //响应器

    RpcContext() : responder_(&ctx_) {}             //构造方法
  };

  /// @brief proto中定义的rpc函数的特化
  typedef RpcContext<msg::ProtoReq, msg::ProtoResp> MsgHandleRpcContext;

  LogicServer() = delete;
  explicit LogicServer(const std::string& location, size_t thread_num);

  ~LogicServer();

  void Run();

 protected:
  // 业务处理函数
  void Handle(MsgHandleRpcContext* rpc_context);

  ThreadPool& GetThreadPool() { return thread_pool_; }

 private:
  // 当前服务器的地址
  std::string server_locaton_;
  // 当前服务器的完成队列，处理异步请求的生命周期
  std::unique_ptr<grpc::ServerCompletionQueue> cq_;
  // 当前服务器的异步服务，处理 gRPC 服务的异步调用
  msg::LogicService::AsyncService service_;
  // 服务器实例
  std::unique_ptr<grpc::Server> server_;
  // 线程池
  ThreadPool thread_pool_;
};

/// @brief kafka消息交付报告回调
class ReportCb : public RdKafka::DeliveryReportCb {
 public:
  void dr_cb (RdKafka::Message &message) override;
};
	
}