#pragma once
#include <mutex>
#include "include/token_verify.h"
#include "proto/route.pb.h"
#include "proto/route.grpc.pb.h"

namespace route {

struct MsgServer {
  std::mutex lock;
  std::string reported;  // 报告的地址
  std::string observed;  // 观察到的地址
  std::set<uint64_t> users; // 在此台服务器上登录的用户
  uint64_t load = 0;
};

// 信令系统：维护用户在线状态、消息推送、业务分发
class RouteServiceImpl : public RouteService::Service {
 public:
  RouteServiceImpl();

  // msg server 验证
  grpc::Status Login(grpc::ServerContext* context, const LoginRequest* request, LoginResponse* response) override;

  // login server获取
  grpc::Status DispatchMsgServer(grpc::ServerContext* context, const DispatchMsgServerRequest* request, DispatchMsgServerResponse* response) override;

  // msg server心跳或注册自己
  grpc::Status MsgServerHeartBeat(grpc::ServerContext* context, const HeartBeatRequest* request, HeartBeatResponse* response) override;

  // transfer 向所有msg推送
  grpc::Status GetMsgServer(grpc::ServerContext* context, const GetMsgServerRequest* request, GetMsgServerResponse* response) override;

 private:
  // 定时器函数：三个token轮转

 private:
  std::mutex server_lock_;
  std::unordered_map<uint64_t, std::shared_ptr<MsgServer>> uid_to_msg_servers_; // uid -> server
  std::unordered_map<std::string, std::shared_ptr<MsgServer>> observed_to_msg_servers_; // observed -> server

  // 不断连接时的token
  std::mutex verifier_lock_;
  std::unique_ptr<TokenVerifier> verifier_;

  // 不断
};

} // namespace varify