#include <algorithm>
#include <gflags/gflags.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include "ws_app.h"
#include "user_mgr.h"
#include "msg.pb.h"
#include "logic_pool.h"

namespace {

DEFINE_string(ws_listen_host, "127.0.0.1", "默认host");
DEFINE_int32(ws_listen_port, 14000, "");

} // namespace

namespace msg::msg_gate {

void UpgradeHandle(uWS::HttpResponse<false> *resp, uWS::HttpRequest *req, struct us_socket_context_t *context) {
  auto uid = req->getQuery("uid");
  auto token = req->getQuery("token");

  spdlog::debug("upgrade: {}", req->getFullUrl());
  if (uid.empty() || token.empty()) {
    resp->close();
	return;
  }
  // TODO: Token认证
  if (token != "nieyang2003@qq.com") {
	spdlog::error("varify token failed: {}", token);
	resp->close();
	return;
  }
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
  // 踢掉其他人
  // TODO: 发送登录请求
}

void MessageHandle(uWS::WebSocket<false, true, UserData> *ws, std::string_view message, uWS::OpCode code) {
  spdlog::debug("MessageHandle: OpCode = {}", (int)code);

  if (code != uWS::OpCode::BINARY) [[unlikely]] {
	spdlog::debug("丢弃无效请求");
	return;
  }
  // 解析数据包
  msg::Protocol proto;
  if (!proto.ParseFromArray(message.data(), message.length())) [[unlikely]] {
    spdlog::error("parse from array error：{}", ws->getRemoteAddress());
	return;
  }
  // 发送到逻辑处理中心
//   if (proto.type() > msg::ProtoType::TYPE_BEGIN_FRIEND_MOD && proto.type() < msg::ProtoType::TYPE_END_FRIENT_MOD) {
//     spdlog::debug("user {} send {}, resend to `friend` logic center", proto.from_uid(), proto.type());
// 	LogicClientPool::Instance()->SendToFriendMod(std::move(proto), proto.from_uid());
//   } else if (proto.type() > msg::ProtoType::TYPE_BEGIN_GROUP_MOD && proto.type() < msg::ProtoType::TYPE_END_GROUP_MOD) {
// 	spdlog::debug("user {} send {}, resend to `group` logic center", proto.from_uid(), proto.type());
// 	LogicClientPool::Instance()->SendToGroupMod(std::move(proto), proto.from_uid());
//   } else if (proto.type() > msg::ProtoType::TYPE_BEGIN_GROUP_MOD && proto.type() < msg::ProtoType::TYPE_END_GROUP_MOD) {
// 	// TODO: 转发给谁，懒得多写一个服务
// 	spdlog::debug("user {} send {}, resend to `msg` logic center", proto.from_uid(), proto.type());
//   } else {
//     spdlog::error("user {} send {}, unknown proto", proto.from_uid(), proto.type());
//   }
}

void DroppedHandle(uWS::WebSocket<false, true, UserData> *ws, std::string_view message, uWS::OpCode code) {
//   SendErrorToClient(ws);
  spdlog::debug("DroppedHandle");
}

void DrainHandle(uWS::WebSocket<false, true, UserData> *ws) {
  spdlog::debug("DrainHandle");
}

void CloseHandle(uWS::WebSocket<false, true, UserData> *ws, int code, std::string_view message) {
  // 关闭连接
  spdlog::info("user {} log out", ws->getUserData()->uid);
  UserManager::Instance()->DelWS(ws->getUserData()->uid);
}

void RunWsApp(std::vector<std::unique_ptr<std::thread>> *threads) {
  for (auto&& thread : *threads) {
	thread.reset(new std::thread([]{
	  uWS::App().ws<UserData>("/*", {
		/* Settings */
        .compression = uWS::CompressOptions(uWS::DEDICATED_COMPRESSOR_4KB | uWS::DEDICATED_DECOMPRESSOR),
        .maxPayloadLength = 100 * 1024 * 1024,
        .idleTimeout = 16,
        .maxBackpressure = 100 * 1024 * 1024,
        .closeOnBackpressureLimit = false,
        .resetIdleTimeoutOnSend = false,
        .sendPingsAutomatically = true,
		/* Handlers */
		.upgrade = UpgradeHandle,
		.open    = OpenHandle,
		.message = MessageHandle,
        .dropped = DroppedHandle, // 超出 maxBackpressure 或 closeOnBackpressureLimit 限制而被丢弃时触发
        .drain   = DrainHandle,   // 当连接缓冲区被清空时触发。此回调函数可用于监控连接的流量和压力情况。
        .ping    = nullptr,       // 当服务器接收到客户端发送的 ping 帧时触发
        .pong    = nullptr,       // 当服务器接收到客户端发送的 pong 帧时触发
        .close   = CloseHandle,   // 当 WebSocket 连接关闭时触发
	  }).listen(FLAGS_ws_listen_host, FLAGS_ws_listen_port, [](auto *listen_socket) {
		std::stringstream ss;
		ss << std::this_thread::get_id();
        if (listen_socket) {
		  spdlog::info("thread {} listening on {}:{}", ss.str(), FLAGS_ws_listen_host, FLAGS_ws_listen_port);
        } else {
		  spdlog::error("thread {} failed to listen on port {}:{}", ss.str(), FLAGS_ws_listen_host, FLAGS_ws_listen_port);
		}
      }).run();
	}));
  }
}

} // msg::msg_gate