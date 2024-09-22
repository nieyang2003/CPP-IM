#include "logic_pool.h"
#include <spdlog/spdlog.h>
#include <uWebSockets/App.h>
#include "user_mgr.h"

namespace msg::msg_gate {

namespace {

DEFINE_uint32(max_logic_clients, 4, "每个logic服务连接池的最大连接数");
DEFINE_string(friend_mod_location, "127.0.0.1:16001", "好友logic模块地址");
DEFINE_string(group_mod_location, "127.0.0.1:16001", "好友logic模块地址");

struct AsyncCallContext {
  msg::MsgInfo resp;
  grpc::ClientContext grpc_context;
  grpc::Status status;
  uint64_t uid;
  std::unique_ptr<grpc::ClientAsyncResponseReader<msg::MsgInfo>> resp_reader;
};

} // namespace

LogicClients::LogicClients(const std::string &url, uint32_t max)
  : url_(url) {
  for (auto i = 0u; i < max; ++i) {
	pool_.emplace(CreateConnection());
  }
}

std::unique_ptr<LogicClients::Client> LogicClients::CreateConnection() {
	auto channel = grpc::CreateChannel(url_, grpc::InsecureChannelCredentials());
	auto p = msg::LogicService::NewStub(channel);
	if (!p) {
		spdlog::error("创建失败");
		exit(1);
	}
	return p;
}

LogicClientPool::LogicClientPool() {
  friend_pool_ = std::make_unique<LogicClients>(FLAGS_friend_mod_location, FLAGS_max_logic_clients);
  group_pool_ = std::make_unique<LogicClients>(FLAGS_group_mod_location, FLAGS_max_logic_clients);
  async_rpc_complete_thread_ = std::make_unique<std::thread>(AsyncRpcComplete);
}

LogicClientPool::~LogicClientPool() {
  if (async_rpc_complete_thread_->joinable()) {
	async_rpc_complete_thread_->join();
	async_rpc_complete_thread_.reset();
  }
}

void LogicClientPool::SendToFriendMod(msg::Protocol&& request, uint64_t uid) {
  AsyncCallContext* rpc_context = new AsyncCallContext;
  rpc_context->uid = uid;
  // 初始化 ClientAsyncResponseReader
  auto stub = friend_pool_->Acquire();
  rpc_context->resp_reader = stub->PrepareAsyncHandle(&rpc_context->grpc_context, request, &cq_);
  friend_pool_->Release(std::move(stub));
  // 发起真正的RPC请求
  rpc_context->resp_reader->StartCall();
  // Finish()方法前两个参数用于指定响应数据的存储位置，第三个tag参数设置为new数据，用于释放
  rpc_context->resp_reader->Finish(&rpc_context->resp, &rpc_context->status, (void*)rpc_context);
}

void LogicClientPool::SendToGroupMod(msg::Protocol&& request, uint64_t uid) {
  AsyncCallContext* rpc_context = new AsyncCallContext;
  rpc_context->uid = uid;
  // 初始化 ClientAsyncResponseReader
  auto stub = group_pool_->Acquire();
  rpc_context->resp_reader = stub->PrepareAsyncHandle(&rpc_context->grpc_context, request, &cq_);
  group_pool_->Release(std::move(stub));
  // 发起真正的RPC请求
  rpc_context->resp_reader->StartCall();
  // Finish()方法前两个参数用于指定响应数据的存储位置，第三个tag参数设置为new数据，用于释放
  rpc_context->resp_reader->Finish(&rpc_context->resp, &rpc_context->status, (void*)rpc_context);
}

void LogicClientPool::AsyncRpcComplete() {
  void* got_tag;
  bool ok = false;

  while (cq_.Next(&got_tag, &ok)) {
	auto* call_context = static_cast<AsyncCallContext*>(got_tag);
	GPR_ASSERT(ok); // 必须真的完成

	if (call_context->status.ok()) {
	  auto ws = UserManager::Instance()->GetWS(call_context->uid);
	  if (ws) [[likely]] {
        std::string output;
	    call_context->resp.SerializeToString(&output);
	    ws->send(output.data());
        spdlog::info("完成");
	  } else {
		spdlog::error("用户 {} 断连", call_context->uid);
	  }
	} else {
	  // 发送错误响应
	  spdlog::error("错误");
	}
	delete call_context; // 删掉new的数据
  }
}

} // namespace msg::msg_gate