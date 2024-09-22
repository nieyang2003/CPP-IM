#include "logic_server.h"
#include "msgid.h"
#include "kafka_producer.h"
#include "include/mysql_pool.h"
#include <Poco/Data/SessionPool.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>
#include <Poco/Data/MySQL/Connector.h>
#include <Poco/Data/DataException.h>
#include <Poco/Data/Transaction.h>
#include <Poco/Data/RecordSet.h>
#include <gflags/gflags.h>
#include <spdlog/spdlog.h>

namespace logic {

DEFINE_string(mysql_conn, "host=127.0.0.1;user=root;password=123456;db=ychat", "MySQL连接配置");

std::mutex lock_;
std::map<std::string, LogicServer::MsgHandleRpcContext*> rpc_contexts_;

// redis存储msg_gate节点、seq
// msg_gate存储msg的ip

bool FriendCtrlHandle(const msg::ProtoReq& request) {
  uint64_t uid = request.from_uid();
  uint64_t friend_uid = request.to_id();
  auto note = request.fctrl().content();

  if (request.fctrl().type() == msg::FriendCtrl::FRIEND_REQ || request.fctrl().type() == msg::FriendCtrl::FRIEND_DEL) {
	// 查询是否有此好友
	int count = 0;
	mysql::CreateStatement() <<
      "SELECT COUNT(*) FROM friend WHERE uid = ? AND friend_uid = ?", 
      Poco::Data::Keywords::into(count), 
      Poco::Data::Keywords::use(uid),
      Poco::Data::Keywords::use(friend_uid), 
      Poco::Data::Keywords::now;

	if (request.fctrl().type() == msg::FriendCtrl::FRIEND_REQ) {
	  // 好友申请
	  if (count > 0) {
		spdlog::error("用户 {} 已经存在好友 {}", uid, friend_uid);
		return false;
	  }
	  // 查询是否正在申请
      int req_count;
	  mysql::CreateStatement() <<
        "SELECT COUNT(*) FROM friend_action_log WHERE (from_uid = ? AND to_uid = ? AND action_type = 0)",
	    Poco::Data::Keywords::into(req_count),
		Poco::Data::Keywords::use(uid),
		Poco::Data::Keywords::use(friend_uid),
		Poco::Data::Keywords::now;
	  if (req_count > 0) {
		spdlog::error("用户 {} 已经添加 {}", uid, friend_uid);
		return false;
	  }
	  // 添加申请记录
	  mysql::CreateStatement() <<
        "INSERT INTO friend_action_log (from_uid, to_uid, action_type) VALUES (?, ?, 0)", 
        Poco::Data::Keywords::use(uid), 
        Poco::Data::Keywords::use(friend_uid),
        Poco::Data::Keywords::now;
      spdlog::info("添加成功, from: {}, to: {}", uid, friend_uid);
	} else {
	  // 删除好友
	  if (count == 0) {
		spdlog::error("用户 {} 不存在好友 {}", uid, friend_uid);
		return false;
	  }
      auto session = mysql::poco_mysql_pool->get();
	  Poco::Data::Transaction transaction(session);
	  session << "DELETE FROM friend WHERE uid = ? AND friend_uid = ?",
                  Poco::Data::Keywords::use(uid),
                  Poco::Data::Keywords::use(friend_uid),
                  Poco::Data::Keywords::now;
	  session << "INSERT INTO friend_action_log (from_uid, to_uid, action_type) VALUES (?, ?, 1)",
	              Poco::Data::Keywords::use(uid),
				  Poco::Data::Keywords::use(friend_uid),
				  Poco::Data::Keywords::now;
	  transaction.commit();
      spdlog::info("删除成功, from: {}, to: {}", uid, friend_uid);
	}
  } else {
	// 查询是否正在申请
    int req_count;
	mysql::CreateStatement() <<
      "SELECT COUNT(*) FROM friend_action_log WHERE (from_uid = ? AND to_uid = ? AND action_type = 0)",
	  Poco::Data::Keywords::into(req_count),
	  Poco::Data::Keywords::use(uid),
	  Poco::Data::Keywords::use(friend_uid),
	  Poco::Data::Keywords::now;
	if (req_count == 0) {
	  spdlog::error("用户 {} 没有申请添加 {}", uid, friend_uid);
	  return false;
	}
    
    if (request.fctrl().type() == msg::FriendCtrl::FRIEND_AUTH_OK) {
      auto session = mysql::poco_mysql_pool->get();
      Poco::Data::Transaction transaction(session);
	  session << "UPDATE friend SET note = ? WHERE uid = ? AND friend_uid = ?",
                  Poco::Data::Keywords::use(note),
                  Poco::Data::Keywords::use(uid),
                  Poco::Data::Keywords::use(friend_uid),
                  Poco::Data::Keywords::now;
	  session << "UPDATE friend SET note = ? WHERE uid = ? AND friend_uid = ?",
                  Poco::Data::Keywords::use(note),
                  Poco::Data::Keywords::use(friend_uid),
                  Poco::Data::Keywords::use(uid),
                  Poco::Data::Keywords::now;
	  session << "UPDATE friend_action_log SET action_type = 2 WHERE from_uid = ? AND to_uid = ?",
	              Poco::Data::Keywords::use(friend_uid),
				  Poco::Data::Keywords::use(uid),
				  Poco::Data::Keywords::now;
	  // 提交事务
      transaction.commit();
	} else {
      mysql::CreateStatement() <<
        "UPDATE friend_action_log SET action_type = 3 WHERE from_uid = ? AND to_uid = ?",
	    Poco::Data::Keywords::use(friend_uid),
		Poco::Data::Keywords::use(uid),
		Poco::Data::Keywords::now;
	}
  }

  return true;
}

bool GroupCtrlHandle(const msg::ProtoReq& request) {
  spdlog::debug("unimplemented handle");
  if (request.gctrl().type() == msg::GroupCtrl::GROUP_CREATE) {
	// 创建群聊
  } else if (request.gctrl().type() == msg::GroupCtrl::GROUP_DELETE) {
    // 删除群聊
  } else if (request.gctrl().type() == msg::GroupCtrl::GROUP_ADD_MEMBER) {
    // 添加好友
  } else if (request.gctrl().type() == msg::GroupCtrl::GROUP_ENTER) {
    // 群组
  } else if (request.gctrl().type() == msg::GroupCtrl::GROUP_EXIT) {
    // 退出
  }
  return false;
}

void logic::LogicServer::Handle(MsgHandleRpcContext* rpc_context) {
  spdlog::debug("from uid = {}, to id = {}", rpc_context->req_.from_uid(), rpc_context->req_.to_id());

  rpc_context->resp_ = GenProtoResp(rpc_context->req_.from_uid()); // 生成key和时间戳
  bool ok = false;
  std::string errstr;

  const auto& request = rpc_context->req_;
  const auto& resp = rpc_context->resp_;
 
  try {
    switch (rpc_context->req_.type()) {
      case msg::ProtoReqType::TypeFriendCtrl: {
        ok = FriendCtrlHandle(request);
        break;
      }
      case msg::ProtoReqType::TypeFriendState: {
        if (request.fstate().type() == msg::FriendState::INPUTING) {
          // TODO: redis找到对应的gate server或发送到push节点，直接发送，不需要走推送
          ok = true;
        }
        break;
      }
      case msg::ProtoReqType::TypeFriendMsg: {
        // TODO: 要撤回的消息是否存在，且在三分钟内
        // TODO: 判断好友是否存在
        ok = true; // 直接推送
        break;
      }
      case msg::ProtoReqType::TypeSystemNotify: {
        // 这个消息应该由系统生成主动写kafka
        spdlog::error("系统消息不能发送到这里");
        ok = false;
        break;
      }
      case msg::ProtoReqType::TypeGroupCtrl: {
        ok = GroupCtrlHandle(request);
        break;
      }
      case msg::ProtoReqType::TypeGroupMsg: {
        // TODO: 判断群组是否存在
        ok = true;
        break;
      }
      case msg::ProtoReqType::TypePull: {
        // 拉取
        spdlog::error("拉取在gate端进行");
        ok = false;
        break;
      }
      case msg::ProtoReqType::TypeLog: {
        switch (request.log().type()) {
          // 在gate设置了键，放到这里更好？
          case msg::Log::LOGIN: {
          // 更新redis
          // 不需要发送到kafka
          // 发送踢人消息
          goto no_send_kafka;
          break;
          }
          case msg::Log::LOGOUT: {
          // 更新redis
          // 清除节点地址
          break;
          }
          case msg::Log::KICKOUT: {
          spdlog::error("被踢的消息不应发到这里");
          break;
          }
        }
        break;
      }
      default: {
        spdlog::error("非法");
        ok = false;
      }
    }
  } catch (Poco::Exception& ex) {
	spdlog::error("Poco Exception: {}", ex.displayText());
	ok = false;
  }

  if (ok) {
	{
	  // 添加
	  std::lock_guard<std::mutex> _(lock_);
	  rpc_contexts_[resp.msgid()] = rpc_context;
	}
	// 设置正确的数据
	auto mq_message = std::make_shared<msg::MqMessage>();
	*mq_message->mutable_handle_result() = resp;
	*mq_message->mutable_origin_req() = request;
	// 将消息放入kafka
	if (!Producer::Instance()->Produce(mq_message)) {
	  {
		// 失败删除
		std::lock_guard<std::mutex> _(lock_);
		rpc_contexts_.erase(mq_message->handle_result().msgid());
	  }
      rpc_context->resp_.set_error_code(0);
      rpc_context->resp_.set_err("Produce error");
	  rpc_context->done_ = true;
	  rpc_context->responder_.Finish(rpc_context->resp_, grpc::Status::OK, rpc_context);
	}
  } else {
	// 错误消息不需要发送到kafka
    rpc_context->resp_.set_error_code(0);
    rpc_context->resp_.set_err("invalid request");
    rpc_context->done_ = true;
	rpc_context->responder_.Finish(rpc_context->resp_, grpc::Status::OK, rpc_context);
  }
  return;

no_send_kafka:
  rpc_context->done_ = true;
  rpc_context->responder_.Finish(rpc_context->resp_, grpc::Status::OK, rpc_context);
}

LogicServer::LogicServer(const std::string &location, size_t thread_num)
  : server_locaton_(location)
  , thread_pool_(thread_num) {
}

LogicServer::~LogicServer() {
  server_->Shutdown();
  cq_->Shutdown();
}

void LogicServer::Run() {
  mysql::InitMysql(FLAGS_mysql_conn);
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


  MsgHandleRpcContext *first = new MsgHandleRpcContext;
  service_.RequestHandle(&first->ctx_, &first->req_, &first->responder_, cq_.get(), cq_.get(), first);

  MsgHandleRpcContext* tag = nullptr;
  bool ok = false;
  // 不断从完成队列中取出请求
  while (true) {
    GPR_ASSERT(cq_->Next((void **)&tag, &ok));
    GPR_ASSERT(ok);

    if (tag->done_) {
      spdlog::debug("done");
      GPR_ASSERT(tag->done_);
      delete tag;
    } else {
      spdlog::debug("running");
      auto *new_ctx = new MsgHandleRpcContext;
      service_.RequestHandle(&new_ctx->ctx_, &new_ctx->req_, &new_ctx->responder_, cq_.get(), cq_.get(), new_ctx);
      thread_pool_.Enqueue([tag, this]() {
  	    Handle(tag);
      });
    }
  }
}

void ReportCb::dr_cb(RdKafka::Message &message) {
  LogicServer::MsgHandleRpcContext* context;
  {
	// 取出
    std::lock_guard<std::mutex> _(lock_);
    auto iter = rpc_contexts_.find(*message.key());
    if (iter == rpc_contexts_.end()) {
  	  spdlog::error("can not found context: {}", *message.key());
  	  return;
    }
    spdlog::info("send push msg `{}` to kafka success", *message.key());
    context = iter->second;
    rpc_contexts_.erase(*message.key()); // 删除对应的消息
  }
  if (message.err()) [[unlikely]] {
    context->resp_.set_error_code(0);
    context->resp_.set_err("kafka error");
	context->done_ = true;
	context->responder_.Finish(context->resp_, grpc::Status::OK, context);
    spdlog::error("交付失败");
  } else {
	context->done_ = true;
	context->responder_.Finish(context->resp_, grpc::Status::OK, context);
    spdlog::info("交付成功");
  }
};

} // namespace logic