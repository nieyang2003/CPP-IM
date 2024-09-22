#include "async_gate_client.h"
#include "push_server.h"
#include <spdlog/spdlog.h>

namespace push {

GateClient::GateClient() {}

void GateClient::Push(PushContext *context, const std::string& gate_address) {
  // 找到对应gate的连接
  decltype(stubs_.begin()) iter;
  {
	std::lock_guard<std::mutex> _(stub_lock_);
	iter = stubs_.find(gate_address);
  }
  // 尝试连接
  if (iter == stubs_.end()) {
    spdlog::debug("");
	// 尝试连接msg_gate
    auto channel = grpc::CreateChannel(gate_address, grpc::InsecureChannelCredentials());
	auto stub = GateService::NewStub(channel);
	if (stub) {
	  spdlog::info("添加msg_gate: {}", gate_address);
	  context->resp_reader = stub->PrepareAsyncPush(&context->client_context, context->req, &cq_);
	  {
		std::lock_guard<std::mutex> _(stub_lock_);
		stubs_[gate_address] = std::move(stub);
	  }
	} else {
	  spdlog::info("连接msg_gate失败: {}", gate_address);
	  // 错误
	  context->status = grpc::Status(grpc::StatusCode::UNAVAILABLE, "连接失败");
      context->done = true;
	  context->responder.Finish(context->resp, context->status, context);
	}
  } else {
    context->resp_reader = iter->second->PrepareAsyncPush(&context->client_context, context->req, &cq_);
  }
  // 向gate发起真正的RPC请求
  context->resp_reader->StartCall();
  // Finish()方法前两个参数用于指定响应数据的存储位置，第三个tag参数设置为new数据，用于释放
  context->resp_reader->Finish(&context->resp, &context->status, (void*)context);
}

GateClient::~GateClient() {
  if (async_rpc_complete_thread_->joinable()) {
  	async_rpc_complete_thread_->join();
  	async_rpc_complete_thread_.reset();
  }
}

void GateClient::Run() {
  if (async_rpc_complete_thread_) [[unlikely]] {
    spdlog::info("complete 线程正在运行");
    return;
  } else {
    spdlog::info("创建 complete 线程");
    async_rpc_complete_thread_ = std::make_unique<std::thread>([&](){AsyncRpcComplete();});
  }
}

void GateClient::AsyncRpcComplete() {
  spdlog::debug("RPC客户端异步线程启动");
  void* got_tag;
  bool ok = false;

  while (cq_.Next(&got_tag, &ok)) {
	auto* context = static_cast<PushContext*>(got_tag);
	GPR_ASSERT(ok); // 必须真的完成
	context->done = true;
	context->responder.Finish(context->resp, context->status, context); // 放入server的完成队列
    // 交给 push server删除
  }
}

} // namepsace push