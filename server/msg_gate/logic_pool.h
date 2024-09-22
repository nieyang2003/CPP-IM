#pragma once
#include <string>
#include <gflags/gflags.h>
#include <grpcpp/grpcpp.h> 
#include "include/singleton.h"
#include "include/connection_pool.h"
#include "msg.pb.h"
#include "msg.grpc.pb.h"

namespace msg::msg_gate {

class LogicClients : public ConnectionPool<msg::LogicService::Stub> {
 public:
  using Client = msg::LogicService::Stub;

  LogicClients(const std::string& url, uint32_t max);
  std::unique_ptr<Client> CreateConnection() override;

 private:
  std::string url_;
};

class LogicClientPool : public Singleton<LogicClientPool> {
  friend class Singleton<LogicClientPool>;
 public:
  void SendToFriendMod(msg::Protocol&& proto, uint64_t uid);
  void SendToGroupMod(msg::Protocol&& proto, uint64_t uid);

 private:
  LogicClientPool();

  ~LogicClientPool();

 private:
  void AsyncRpcComplete();

 private:
  // 业务
  std::unique_ptr<LogicClients> friend_pool_;
  std::unique_ptr<LogicClients> group_pool_;

  // 异步任务
  std::unique_ptr<std::thread> async_rpc_complete_thread_;
  grpc::CompletionQueue cq_;
};

} // namespace msg::msg_gate