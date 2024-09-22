// grpc service实现，处理push

#pragma once
#include "msg.pb.h"
#include "msg.grpc.pb.h"
#include "user_mgr.h"

namespace msg::msg_gate {

class MsgServiceImpl : public GateService::Service {
 public:
  // 将数据推送到用户
  grpc::Status Push(grpc::ServerContext* context, const PushMsg* request, Empty* response) override {
    // get ws然后发送
    auto ws = UserManager().Instance()->GetWS(request->proto().to_id());
    if (ws) [[likely]] {
	  std::string data;
	  request->SerializeToString(&data);
	  ws->send(data, uWS::OpCode::BINARY /*, false, true */);
    }

    return grpc::Status::OK; // always
  }
};

// TODO: token

} // namespace msg