// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: varify.proto

#include "varify.pb.h"
#include "varify.grpc.pb.h"

#include <functional>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/message_allocator.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>
namespace varify {

static const char* VarifyService_method_names[] = {
  "/varify.VarifyService/GetVarifyCode",
};

std::unique_ptr< VarifyService::Stub> VarifyService::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< VarifyService::Stub> stub(new VarifyService::Stub(channel, options));
  return stub;
}

VarifyService::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options)
  : channel_(channel), rpcmethod_GetVarifyCode_(VarifyService_method_names[0], options.suffix_for_stats(),::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status VarifyService::Stub::GetVarifyCode(::grpc::ClientContext* context, const ::varify::GetVarifyCodeRequest& request, ::varify::GetVarifyCodeResponse* response) {
  return ::grpc::internal::BlockingUnaryCall< ::varify::GetVarifyCodeRequest, ::varify::GetVarifyCodeResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), rpcmethod_GetVarifyCode_, context, request, response);
}

void VarifyService::Stub::async::GetVarifyCode(::grpc::ClientContext* context, const ::varify::GetVarifyCodeRequest* request, ::varify::GetVarifyCodeResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc::internal::CallbackUnaryCall< ::varify::GetVarifyCodeRequest, ::varify::GetVarifyCodeResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_GetVarifyCode_, context, request, response, std::move(f));
}

void VarifyService::Stub::async::GetVarifyCode(::grpc::ClientContext* context, const ::varify::GetVarifyCodeRequest* request, ::varify::GetVarifyCodeResponse* response, ::grpc::ClientUnaryReactor* reactor) {
  ::grpc::internal::ClientCallbackUnaryFactory::Create< ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(stub_->channel_.get(), stub_->rpcmethod_GetVarifyCode_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::varify::GetVarifyCodeResponse>* VarifyService::Stub::PrepareAsyncGetVarifyCodeRaw(::grpc::ClientContext* context, const ::varify::GetVarifyCodeRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc::internal::ClientAsyncResponseReaderHelper::Create< ::varify::GetVarifyCodeResponse, ::varify::GetVarifyCodeRequest, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(channel_.get(), cq, rpcmethod_GetVarifyCode_, context, request);
}

::grpc::ClientAsyncResponseReader< ::varify::GetVarifyCodeResponse>* VarifyService::Stub::AsyncGetVarifyCodeRaw(::grpc::ClientContext* context, const ::varify::GetVarifyCodeRequest& request, ::grpc::CompletionQueue* cq) {
  auto* result =
    this->PrepareAsyncGetVarifyCodeRaw(context, request, cq);
  result->StartCall();
  return result;
}

VarifyService::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      VarifyService_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< VarifyService::Service, ::varify::GetVarifyCodeRequest, ::varify::GetVarifyCodeResponse, ::grpc::protobuf::MessageLite, ::grpc::protobuf::MessageLite>(
          [](VarifyService::Service* service,
             ::grpc::ServerContext* ctx,
             const ::varify::GetVarifyCodeRequest* req,
             ::varify::GetVarifyCodeResponse* resp) {
               return service->GetVarifyCode(ctx, req, resp);
             }, this)));
}

VarifyService::Service::~Service() {
}

::grpc::Status VarifyService::Service::GetVarifyCode(::grpc::ServerContext* context, const ::varify::GetVarifyCodeRequest* request, ::varify::GetVarifyCodeResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace varify

