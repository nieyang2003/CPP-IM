#pragma once
#include <uWebSockets/WebSocket.h>
#include <stdint.h>
#include <unordered_map>
#include <mutex>
#include  "include/singleton.h"

namespace msg::msg_gate {

struct UserData {
  uint64_t uid = 0; // 用户uid
  std::string device_id; // 登录地址
};

/// @brief 本台msg gate server登录的用户数据
class UserManager : public Singleton<UserManager> {
  friend class Singleton<UserManager>;
 public:
  ~UserManager() override = default;

  void AddWS(uint64_t uid, uWS::WebSocket<false, true, UserData>* ws) {
    std::lock_guard<std::mutex> _(lock_);
	sockets_[uid] = ws;
	ws->getUserData()->uid = uid;
  }

  void DelWS(uint64_t uid) {
    std::lock_guard<std::mutex> _(lock_);
	sockets_.erase(uid);
  }

  uWS::WebSocket<false, true, UserData>* GetWS(uint64_t uid) {
	std::lock_guard<std::mutex> _(lock_);
	auto iter = sockets_.find(uid);
	if (iter == sockets_.end()) {
		return nullptr;
	}
	return iter->second;
  }

  bool IsLogin(uint64_t uid) const {
    std::lock_guard<std::mutex> _(lock_);
	return !!sockets_.count(uid);
  }

 private:
  mutable std::mutex lock_;
  std::unordered_map<uint64_t, uWS::WebSocket<false, true, UserData>*> sockets_;
};

} // namespace msg::msg_gate