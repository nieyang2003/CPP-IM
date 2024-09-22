#pragma once
#include "./varify.pb.h"
#include "./varify.grpc.pb.h"

namespace varify {

class VarifyServiceImpl : public VarifyService::Service {
public:
  grpc::Status GetVarifyCode(grpc::ServerContext* context,
    const GetVarifyCodeRequest* request, GetVarifyCodeResponse* response) override;
};

} // namespace varify