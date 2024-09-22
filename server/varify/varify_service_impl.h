#pragma once
#include "proto/varify.pb.h"
#include "proto/varify.grpc.pb.h"

namespace varify {

class VarifyServiceImpl : public VarifyService::Service {
public:
  grpc::Status GetVarifyCode(grpc::ServerContext* context,
    const GetVarifyCodeRequest* request, GetVarifyCodeResponse* response) override;
};

} // namespace varify