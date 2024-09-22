#include "logic_server.h"
#include "msgid.h"

namespace logic {

grpc::Status logic::LogicServer::Handle(grpc::ServerContext *context, const msg::Protocol &request, msg::MsgInfo *response) {
  response->set_timestamp(0); // error
  auto [key, tm] = GenMsgidAndUnixTime(request.from_uid());

  switch (request.type()) {
	case msg::ProtoType::TypeFriendCtrl: {
	  break;
	}
	case msg::ProtoType::TypeFriendState: {
	  break;
	}
	case msg::ProtoType::TypeFriendMsg: {
	  break;
	}
	case msg::ProtoType::TypeSystemNotify: {
	  break;
	}
	case msg::ProtoType::TypeGroupCtrl: {
	  break;
	}
	case msg::ProtoType::TypeGroupMsg: {
	  break;
	}
	case msg::ProtoType::TypePull: {
	  break;
	}
	case msg::ProtoType::TypeLog: {
	  break;
	}
  }
}

void LogicServer::Run(size_t thread_num) {
  // 服务器构建器
  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_locaton_, grpc::InsecureServerCredentials());
  // 注册服务
  builder.RegisterService(&service_);
  // 为当前服务器创建完成队列
  cq_ = builder.AddCompletionQueue();
  // 构建并启动服务器
  server_ = builder.BuildAndStart();
  spdlog::info("server start on {}", server_locaton_);
  // 创建请求上下文
  MsgHandleRpcContext* msg_handle_context = new MsgHandleRpcContext;
  // 第一次注册，参数从前到后分别是：rpc服务上下文，rpc请求对象，异步响应器，新的rpc请求使用的完成队列，通知完成使用的完成队列，唯一标识tag标识当前这次请求的上下文
  service_.RequestHandle(&msg_handle_context->ctx_,
	                     &msg_handle_context->req_,
						 &msg_handle_context->responder_, 
						 cq_.get(), cq_.get(), msg_handle_context);
  // 创建线程池，用于放入任务
  ThreadPool thread_pool(thread_num);
  // 不断从完成队列中取出请求
  while (true) {
    MsgHandleRpcContext* tag = nullptr;
    bool ok = false;
    GPR_ASSERT(cq_->Next((void **)&tag, &ok));
    GPR_ASSERT(ok);
    // 如果完成了
    if (tag->done_) {
  	  delete tag;
  	  continue;
    }
    // 没有完成
    MsgHandleRpcContext *hadler_context = new MsgHandleRpcContext;
    // 注册新请求，参数从前到后分别是：rpc服务上下文，rpc请求对象，异步响应器，新的rpc请求使用的完成队列，通知完成使用的完成队列，唯一标识tag标识当前这次请求的上下文
    service_.RequestHandle(&hadler_context->ctx_, &hadler_context->req_, &hadler_context->responder_, cq_.get(), cq_.get(), hadler_context);
    thread_pool.Enqueue([hadler_context, this](){
  	  auto status = Handle(&hadler_context->ctx_, hadler_context->req_, &hadler_context->resp_);
  	  hadler_context->done_ = true;
  	  hadler_context->responder_.Finish(hadler_context->resp_, status, hadler_context);
    });
  }
}

} // namespace logic