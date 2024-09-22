#include "push_server.h"
#include <gflags/gflags.h>
#include <spdlog/spdlog.h>
#include "include/redis_pool.h"

namespace push {

namespace {
DEFINE_string(user_redis_key_prefix, "ychat::user::", "");
}

PushServer::PushServer(const std::string &location, size_t thread_num)
  : server_locaton_(location)
  , thread_pool_(thread_num) {}

PushServer::~PushServer() {
  server_->Shutdown();
  cq_->Shutdown();
}

void PushServer::Run() {
  gate_.Run();
  // 服务器构建器
  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_locaton_, grpc::InsecureServerCredentials());
  // 注册服务
  builder.RegisterService(&service_);
  // 为当前服务器创建完成队列
  cq_ = builder.AddCompletionQueue();
  // 构建并启动服务器
  server_ = builder.BuildAndStart();
  spdlog::info("server start on {}", server_locaton_);


  PushContext* push_context = new PushContext;
  service_.RequestPush(&push_context->ctx, &push_context->req, &push_context->responder, cq_.get(), cq_.get(), push_context);

  // 不断从完成队列中取出请求
  PushContext* tag = nullptr;
  bool ok = false;
  while (true) {
    GPR_ASSERT(cq_->Next((void **)&tag, &ok));
    GPR_ASSERT(ok);

    if (tag->done) {
      spdlog::debug("done");
      GPR_ASSERT(tag->done);
    } else {
      PushContext *new_ctx = new PushContext;
      service_.RequestPush(&new_ctx->ctx, &new_ctx->req, &new_ctx->responder, cq_.get(), cq_.get(), new_ctx);

      thread_pool_.Enqueue([tag, this]() {
        // 查找gate地址
	    auto value = redis::redis_pool->hget(fmt::format("{}{}", FLAGS_user_redis_key_prefix, tag->req.to_uid()), "gate");
        // 是否找到gate server
	    if (value.has_value() && !value.value().empty()) {
          spdlog::debug("推送地址：{}", value.value());
	  	  gate_.Push(tag, value.value());
	    } else {
          spdlog::error("the user `{}` is offline", tag->req.to_uid());
	  	  tag->done = true;
	  	  tag->responder.Finish(tag->resp, grpc::Status(grpc::StatusCode::NOT_FOUND, "用户未上线"), tag);
	    }
      });
    }
  }
}

}