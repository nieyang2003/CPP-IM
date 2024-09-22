#include "gate_server.h"
#include <gflags/gflags.h>
#include <spdlog/spdlog.h>

namespace msg::msg_gate {

namespace {
struct PushContext {
  bool done = false;
  grpc::ServerContext ctx;                            // rpc服务的上下文信息
  msg::PushReq req;                                   //请求数据类型
  msg::PushResp resp;                                    //响应数据类型
  grpc::ServerAsyncResponseWriter<msg::PushResp> responder; //响应器
  PushContext() : responder(&ctx) {}
};
}

GateServer::~GateServer() {
  server_->Shutdown();
  cq_->Shutdown();
}

void GateServer::Run(const std::string& location, std::shared_ptr<ThreadPool> pool) {
  // 服务器构建器
  grpc::ServerBuilder builder;
  builder.AddListeningPort(location, grpc::InsecureServerCredentials());
  // 注册服务
  builder.RegisterService(&service_);
  // 为当前服务器创建完成队列
  cq_ = builder.AddCompletionQueue();
  // 构建并启动服务器
  server_ = builder.BuildAndStart();
  spdlog::info("server start on {}", location);



  PushContext* push_context = new PushContext;
  service_.RequestPush(&push_context->ctx, &push_context->req, &push_context->responder,  cq_.get(), cq_.get(), push_context);
  // 不断从完成队列中取出请求
  PushContext* tag = nullptr;
  bool ok = false;
  while (true) {
    GPR_ASSERT(cq_->Next((void **)&tag, &ok));
    GPR_ASSERT(ok);

	if (tag->done) {
      spdlog::debug("done");
	  delete tag;
	  continue;
	} else {
      spdlog::debug("running");
      PushContext *new_ctx = new PushContext;
      service_.RequestPush(&new_ctx->ctx, &new_ctx->req, &new_ctx->responder, cq_.get(), cq_.get(), new_ctx);

      pool->Enqueue([tag, this]() {
        // TODO: 主动断开连接
	    grpc::Status status;
	    auto ws = UserManager().Instance()->GetWS(tag->req.to_uid());
        if (ws) [[likely]] {
	      std::string data;
	      tag->req.SerializeToString(&data);
	      ws->send(data, uWS::OpCode::BINARY /*, false, true */);
	      status = grpc::Status::OK;
        } else {
          status = grpc::Status(grpc::StatusCode::NOT_FOUND, "找不到用户");
	    }
	    tag->done = true;
	    tag->responder.Finish(tag->resp, status, tag);
      });
    }
  }
}

} // namespace msg::msg_gate