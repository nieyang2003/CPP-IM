#include "route_service_impl.h"
#include <gflags/gflags.h>
#include <optional>

namespace route {

namespace {

DEFINE_string(msg_server_token, "nieyang2003@qq.com", "心跳");
DEFINE_string(msg_servers, "127.0.0.1:14000", "默认的msg server");

std::optional<std::pair<std::string_view, std::string_view>> TryGetHostPort(const std::string_view& location) {
  auto pos = location.find_last_of(':');
  if (pos == std::string::npos || pos == 0) {
	return std::nullopt;
  }
  std::string_view host = location.substr(0, pos);
  std::string_view port = location.substr(pos + 1);
  return {{host, port}};
}

// 获得观测地址
std::optional<std::string> GetObserved(grpc::ServerContext *context, const std::string& reported) {
  auto&& peer = context->peer();
  // 划分出host和端口
  auto reported_ip_port = TryGetHostPort(reported);
  if (!reported_ip_port) return std::nullopt;
  auto peer_ip_port = TryGetHostPort(std::string_view(peer.substr(5)));
  if (!peer_ip_port) return std::nullopt;
  // 比对
  if (reported_ip_port->first != peer_ip_port->first) {
	return std::nullopt;
  }
  return std::string(peer_ip_port->first) + std::string(reported_ip_port->first);
}

} // namespace

RouteServiceImpl::RouteServiceImpl() {
  verifier_ = MakeTokenVerifier(FLAGS_msg_server_token);
  auto locations = Split(FLAGS_msg_servers, '|', false);
  for (auto&& location : locations) {
	auto s = std::make_shared<MsgServer>();
	s->observed = s->reported = location;
	observed_to_msg_servers_[std::string(location)] = s;
  }
}

grpc::Status RouteServiceImpl::Login(grpc::ServerContext *context, const LoginRequest *request, LoginResponse *response) {
  // TODO:
  // 是否存在此服务器，否则直接拒绝
//   auto uid = request->uid();
//   auto &&token = request->token();
  // 验证user的token是否通过，通过返回ok就好
  // 
  (void)request->token();
  return grpc::Status::OK;
}

grpc::Status RouteServiceImpl::DispatchMsgServer(grpc::ServerContext *context, const DispatchMsgServerRequest *request, DispatchMsgServerResponse *response) {
  // round算法
  // 找最近msg_server
  // 找负载最低的server

  uint64_t min_load = UINT64_MAX;
  std::shared_ptr<MsgServer> target = nullptr; // 防止释放

  std::lock_guard _(server_lock_);
  for (auto&& [_, msg_server] : observed_to_msg_servers_) {
	spdlog::debug("{}: {}", _, msg_server->load);
    if (msg_server->load < min_load) {
	  min_load = msg_server->load;
	  target = msg_server;
	}
  }

  if (target) [[likely]] {
	// TODO: token
  	response->set_location(target->observed);
	response->set_token("nieyang2003@qq.com");
	return grpc::Status::OK;
  } else {
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "没有msg server");
  }
}

grpc::Status RouteServiceImpl::MsgServerHeartBeat(grpc::ServerContext *context, const HeartBeatRequest *request, HeartBeatResponse *response) {
  // 验证token

  // 地址操作
  std::string observed;

  // 更新信息
  std::set<uint64_t> users;
  std::shared_ptr<MsgServer> server{};

  {
    std::lock_guard _(server_lock_);
    auto iter = observed_to_msg_servers_.find(observed);
    if (iter == observed_to_msg_servers_.end()) [[unlikely]] {
      spdlog::info("发现新的msg server节点: {}", observed);
      observed_to_msg_servers_[observed] = std::make_shared<MsgServer>();
    }
    for (auto uid : request->uids()) {
      users.insert(uid);
      uid_to_msg_servers_[uid] = iter->second;
    }
    server = observed_to_msg_servers_[observed];
  }
  {
    std::lock_guard _(server->lock);
	server->users.swap(users);
  }

  // 返回token
  // 客户端登录时没有给token，msg server不接受


  return grpc::Status::OK;
}

grpc::Status RouteServiceImpl::GetMsgServer(grpc::ServerContext *context, const GetMsgServerRequest *request, GetMsgServerResponse *response) {
  // 验证token

  // 
  std::lock_guard _(server_lock_);
  auto iter = uid_to_msg_servers_.find(request->uid());
  if (iter != uid_to_msg_servers_.end()) [[likely]] {
	response->add_locations(iter->second->observed);
  } else {
    spdlog::trace("can not found server of uid-{}", request->uid());
	for (auto&& [observed,_ ] : observed_to_msg_servers_) {
	  response->add_locations(observed); // 全部放入
	}
  }

  return grpc::Status::OK;
}

}