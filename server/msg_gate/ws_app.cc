#include <algorithm>
#include <gflags/gflags.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <sw/redis++/redis++.h>
#include <include/redis_pool.h>
#include "ws_app.h"
#include "user_mgr.h"
#include "async_logic_client.h"

namespace {

DEFINE_string(ws_host, "127.0.0.1", "ws host");
DEFINE_int32(ws_port, 10010, "ws端口");

DEFINE_string(redis_user_prefix, "ychat::user::", "用户redis键");
DEFINE_string(redis_user_token_prefix, "ychat::user::token::", "用户登录gate的token");

std::shared_ptr<ThreadPool> pool_;
std::string gate_location_;

} // namespace

namespace msg::msg_gate {

/// @brief 同步发送错误响应
/// @param ws 
void SendErrorResp(uWS::WebSocket<false, true, UserData> *ws, int err_code, const std::string& err_str) {
  msg::ProtoResp resp;
  resp.set_error_code(err_code);
  resp.set_err(err_str);
  auto err = resp.SerializeAsString();
  ws->send(err, uWS::OpCode::BINARY);
}

void UpgradeHandle(uWS::HttpResponse<false> *resp, uWS::HttpRequest *req, struct us_socket_context_t *context) {
  auto uid = req->getQuery("uid");
  auto token = req->getQuery("token");

  spdlog::debug("upgrade: {}", req->getFullUrl());
  if (uid.empty() || token.empty()) {
    resp->close();
	return;
  }
  // token认证后删除token并写入gate地址
  auto value = redis::redis_pool->get(fmt::format("{}{}", FLAGS_redis_user_token_prefix, uid));
  if (!value.has_value() || token != value.value()) {
    spdlog::error("varify token failed: {}", token);
    resp->close();
    return;
  }
  // 同步逻辑踢掉别人后再让用户上线
  auto gate = redis::redis_pool->hget(fmt::format("{}{}", FLAGS_redis_user_prefix, uid), "gate");
  if (gate.has_value() && !gate.value().empty()) {
    // TODO: 发送login消息到logic来踢掉别人
    spdlog::info("user `{}` is online, kick it out on {}", uid, gate.value());
  }
  pool_->Enqueue([&](){
    redis::redis_pool->hset(fmt::format("{}{}", FLAGS_redis_user_prefix, uid), {"gate", gate_location_});
     redis::redis_pool->del(fmt::format("{}{}", FLAGS_redis_user_token_prefix, uid));
  });
  // 手动升级
  resp->upgrade<UserData>({std::stoul(std::string(uid))},
    req->getHeader("sec-websocket-key"),
    req->getHeader("sec-websocket-protocol"),
    req->getHeader("sec-websocket-extensions"),
    context);
}

void OpenHandle(uWS::WebSocket<false, true, UserData> *ws) {
  // 通过了token验证，可能多个同时通过
  spdlog::info("user {} logged in this server", ws->getUserData()->uid);
  UserManager::Instance()->AddWS(ws->getUserData()->uid, ws);
}

/// @brief 收到完整数据包
void MessageHandle(uWS::WebSocket<false, true, UserData> *ws, std::string_view message, uWS::OpCode code) {
  spdlog::debug("MessageHandle: OpCode = {}", (int)code);

  if (code != uWS::OpCode::BINARY) [[unlikely]] {
	spdlog::debug("丢弃无效请求");
	return;
  }
  // 解析数据包
  msg::ProtoReq req;
  if (!req.ParseFromArray(message.data(), message.length())) [[unlikely]] {
    spdlog::error("parse from array error：{}", ws->getRemoteAddress());
	return;
  }
  // 过滤掉空消息
  if (req.type() == msg::ProtoReqType::TypeEmpty) [[unlikely]] {
    return;
  }
  // pull请求直接读数据库
  if (req.type() == msg::ProtoReqType::TypePull) {
    // TODO: 
    if (req.pull().type() == msg::Pull::PULL_OFFLINE) { // 离线消息
      auto value = redis::redis_pool->hget(fmt::format("{}{}", FLAGS_redis_user_prefix, ws->getUserData()->uid), "seq");
      if (!value.has_value()) {
        spdlog::error("not found the user {}'s `seq`", ws->getUserData()->uid);
        SendErrorResp(ws, 1001, "seq not found");
      }
    //   auto server_seq = std::stoul(value.value());
    //   auto user_seq = req.pull().local_seq();
      // TODO: 处理条数
    } else if (req.pull().type() == msg::Pull::PULL_HISTORY) { // 历史消息
      // TODO: 如何判断从哪里漫游历史消息
    }
    return;
  }
  // 发送到logic节点处理
  LogicClient::Instance()->SendToLogic(std::move(req), ws->getUserData()->uid);
}

void DroppedHandle(uWS::WebSocket<false, true, UserData> *ws, std::string_view message, uWS::OpCode code) {
  spdlog::debug("DroppedHandle");
}

void DrainHandle(uWS::WebSocket<false, true, UserData> *ws) {
  spdlog::debug("DrainHandle");
}

/// @brief 关闭连接
void CloseHandle(uWS::WebSocket<false, true, UserData> *ws, int code, std::string_view message) {
  redis::redis_pool->hdel(fmt::format("{}{}", FLAGS_redis_user_prefix, ws->getUserData()->uid), "gate"); // 删除gate地址
  spdlog::info("user {} log out, ws {}: {}", ws->getUserData()->uid, code, message);
  UserManager::Instance()->DelWS(ws->getUserData()->uid);
}

void RunWsApp(std::shared_ptr<ThreadPool> pool, const std::string& gate_location) {
  pool_ = pool; gate_location_ = gate_location; // !

  uWS::App().ws<UserData>("/*", {
	/* Settings */
    .compression = uWS::CompressOptions(uWS::DEDICATED_COMPRESSOR_4KB | uWS::DEDICATED_DECOMPRESSOR),
    .maxPayloadLength = 1024 * 1024 * 1024,
    .idleTimeout = 16,
    .maxBackpressure = 1024 * 1024 * 1024,
    .closeOnBackpressureLimit = false,
    .resetIdleTimeoutOnSend = false,
    .sendPingsAutomatically = true,
	/* 异步Handlers */
	.upgrade = UpgradeHandle,
	.open    = [](auto *ws){ pool_->Enqueue([&](){ OpenHandle(ws); }); },
	.message = [](auto *ws, auto message, auto code){ pool_->Enqueue([&](){ MessageHandle(ws, message, code); }); },
    .dropped = [](auto *ws, auto message, auto code){ pool_->Enqueue([&](){ DroppedHandle(ws, message, code); }); }, // 超出 maxBackpressure 或 closeOnBackpressureLimit 限制而被丢弃时触发
    .drain   = [](auto *ws){ pool_->Enqueue([&](){ DrainHandle(ws); }); },   // 当连接缓冲区被清空时触发。此回调函数可用于监控连接的流量和压力情况。
    .ping    = nullptr,       // 当服务器接收到客户端发送的 ping 帧时触发
    .pong    = nullptr,       // 当服务器接收到客户端发送的 pong 帧时触发
    .close   = [](auto *ws, auto code, auto message){ pool_->Enqueue([&](){ CloseHandle(ws, code, message); }); },   // 当 WebSocket 连接关闭时触发
  }).listen(FLAGS_ws_host, FLAGS_ws_port, [](auto *listen_socket) {
	std::stringstream ss;
	ss << std::this_thread::get_id();
    if (listen_socket) {
	  spdlog::info("thread {} listening on {}:{}", ss.str(), FLAGS_ws_host, FLAGS_ws_port);
    } else {
	  spdlog::error("thread {} failed to listen on port {}:{}", ss.str(), FLAGS_ws_host, FLAGS_ws_port);
	}
  }).run();

  pool_.reset();
  redis::redis_pool.reset();
}

} // msg::msg_gate