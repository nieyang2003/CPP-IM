#include "async_logic_client.h"
#include <spdlog/spdlog.h>
#include <uWebSockets/App.h>
#include "user_mgr.h"

namespace msg::msg_gate {

namespace {

DEFINE_uint32(max_clients, 4, "每个logic服务连接池的最大连接数");
DEFINE_string(logic_service_address, "127.0.0.1:10001", "好友logic模块地址");

struct AsyncCallContext {
  msg::ProtoResp resp;
  grpc::ClientContext grpc_context;
  grpc::Status status;
  uint64_t uid;
  std::unique_ptr<grpc::ClientAsyncResponseReader<msg::ProtoResp>> resp_reader;
};

} // namespace

LogicStubs::LogicStubs(const std::string &url, uint32_t max)
  : url_(url) {
  for (auto i = 0u; i < max; ++i) {
	pool_.emplace(CreateConnection()); // 创建连接，每个连接也可以复用
  }
}

std::unique_ptr<LogicService::Stub> LogicStubs::CreateConnection() {
  auto channel = grpc::CreateChannel(url_, grpc::InsecureChannelCredentials());
  auto p = msg::LogicService::NewStub(channel);
  if (!p) {
  	spdlog::error("创建失败");
  	exit(1);
  }
  return p;
}

LogicClient::LogicClient() {
  stubs_ = std::make_unique<LogicStubs>(FLAGS_logic_service_address, FLAGS_max_clients);
  async_rpc_complete_thread_ = std::make_unique<std::thread>([&](){
    AsyncRpcComplete();
  });
}

LogicClient::~LogicClient() {
  if (async_rpc_complete_thread_->joinable()) {
	async_rpc_complete_thread_->join();
	async_rpc_complete_thread_.reset();
  }
}

void LogicClient::SendToLogic(msg::ProtoReq&& request, uint64_t uid) {
  spdlog::info("转发给服务器，uid = {}", uid);
  AsyncCallContext* rpc_context = new AsyncCallContext;
  rpc_context->uid = uid;
  // 初始化 ClientAsyncResponseReader
  auto stub = stubs_->Acquire();
  rpc_context->resp_reader = stub->PrepareAsyncHandle(&rpc_context->grpc_context, request, &cq_);
  stubs_->Release(std::move(stub));
  // 发起真正的RPC请求
  rpc_context->resp_reader->StartCall();
  // Finish()方法前两个参数用于指定响应数据的存储位置，第三个tag参数设置为new数据，用于释放
  rpc_context->resp_reader->Finish(&rpc_context->resp, &rpc_context->status, (void*)rpc_context);
}

void LogicClient::AsyncRpcComplete() {
  void* got_tag;
  bool ok = false;

  while (cq_.Next(&got_tag, &ok)) {
	auto* call_context = static_cast<AsyncCallContext*>(got_tag);
	GPR_ASSERT(ok); // 必须真的完成

	spdlog::info("收到logic响应，发送回用户 {}，result: {}", call_context->uid,
      call_context->status.ok() ? call_context->resp.msgid() : call_context->status.error_message());

    auto ws = UserManager::Instance()->GetWS(call_context->uid);
    if (ws) [[likely]] {
      auto resp = call_context->resp.SerializeAsString();
	  ws->send(resp); // TODO: 异步发送，一个线程可能不够
    } else {
      spdlog::error("用户 {} 断连", call_context->uid);
    }

	delete call_context; // 删掉new的数据
  }
}

} // namespace msg::msg_gate