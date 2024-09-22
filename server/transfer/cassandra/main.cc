#include <cassandra.h>
#include <gflags/gflags.h>
#include <spdlog/spdlog.h>
#include <grpcpp/grpcpp.h> 
#include "include/redis_pool.h"
#include "include/mysql_pool.h"
#include "include/connection_pool.h"
#include "include/singleton.h"
#include "transfer/kafka_consumer.h"
#include "proto/push.grpc.pb.h"

DEFINE_string(cluster_host, "127.0.0.1", "卡桑德拉host");
DEFINE_int32(cluster_port, 9042, "卡桑德拉端口");
DEFINE_uint32(num_sessions, 4, "卡桑德拉连接池连接数");
DEFINE_string(groupid, "cassandra", "卡夫卡消费者组");
DEFINE_string(topic, "ychat", "卡夫卡topic");

DEFINE_string(push_server_address, "127.0.0.1:10002", "push节点地址");
DEFINE_string(redis_user_prefix, "ychat::user::", "用户redis键前缀");
DEFINE_string(redis_host, "127.0.0.1", "用户redis键前缀");
DEFINE_string(mysql_conn, "host=127.0.0.1;user=root;password=123456;db=ychat", "用户redis键前缀");

class CassSeessionPool : public ConnectionPool<CassSession, void(*)(CassSession*)>, public Singleton<CassSeessionPool> {
  friend class Singleton<CassSeessionPool>;
 private:
  CassSeessionPool()
  	: cluster_(cass_cluster_new(), &cass_cluster_free) {
    cass_cluster_set_contact_points(cluster_.get(), FLAGS_cluster_host.c_str());
	cass_cluster_set_port(cluster_.get(), FLAGS_cluster_port);

	for (size_t i = 0; i < FLAGS_num_sessions; ++i) {
  	  pool_.push(std::move(CreateConnection()));
    }
  }

  /// @brief 创建连接
  std::unique_ptr<CassSession,void(*)(CassSession*)> CreateConnection() override {
    std::unique_ptr<CassSession, void(*)(CassSession*)> session(cass_session_new(), &cass_session_free);
	std::unique_ptr<CassFuture, void(*)(CassFuture*)> connect_future(cass_session_connect(session.get(), cluster_.get()), &cass_future_free);

    // 等待连接完成
    CassError rc = cass_future_error_code(connect_future.get());

    if (rc != CASS_OK) {
        // 连接失败，处理错误
        const char* message;
        size_t message_length;
        cass_future_error_message(connect_future.get(), &message, &message_length);
		spdlog::error("connect to {} failed", std::string(message, message_length));
		exit(1);
    }
	spdlog::debug("创建CassSession成功");

    // 返回会话对象
    return session;
  }

 public:

  /// @brief 成功推送到用户时的回调函数，异步删除用户收件箱的数据
  void OnPushSuccess(uint64_t uid, int64_t seq) {
	const char* query = "DELETE FROM ychat.user_inbox WHERE user_id = ? AND seq = ?;";
	std::unique_ptr<CassStatement, void(*)(CassStatement*)> statement(cass_statement_new(query, 2), &cass_statement_free);
	cass_statement_bind_int64(statement.get(), 0, static_cast<int64_t>(uid));
    cass_statement_bind_int64(statement.get(), 1, static_cast<int64_t>(seq));
	auto session = Acquire();
    std::unique_ptr<CassFuture, void(*)(CassFuture*)> future(cass_session_execute(session.get(), statement.get()), &cass_future_free);
	Release(std::move(session));
	// 设置删除完成时的回调函数
    cass_future_set_callback(future.get(), [](CassFuture* future, void* data){
      if (cass_future_error_code(future) == CASS_OK) [[likely]] {
        spdlog::info("删除成功");
      } else {
        spdlog::error("删除失败");
      }
    }, nullptr);
  }

  /// @brief 异步写入收件箱
  void AsyncWriteInbox(const std::string& msgid, uint64_t uid, int64_t seq) {
	const char* query = "INSERT INTO ychat.user_inbox (uid, seq, msgid) VALUES (?, ?, ?);";
	std::unique_ptr<CassStatement, void(*)(CassStatement*)> statement(cass_statement_new(query, 3), &cass_statement_free);
	cass_statement_bind_int64(statement.get(), 0, (int64_t)uid); // 雪花算法生成，第一位不为0
    cass_statement_bind_int64(statement.get(), 1, (int64_t)seq); // 应该用不了这么多，到负数了都
    cass_statement_bind_string(statement.get(), 2, msgid.c_str());
    // 异步写入
	auto session = Acquire();
	std::unique_ptr<CassFuture, void(*)(CassFuture*)> future(cass_session_execute(session.get(), statement.get()), &cass_future_free);
	Release(std::move(session));
    // 设置写入完成时的回调函数
    cass_future_set_callback(future.get(), [](CassFuture* future, void* data){
      if (cass_future_error_code(future) == CASS_OK) [[likely]] {
        spdlog::info("写入ychat.user_inbox成功");
	  } else {
        spdlog::error("写入ychat.user_inbox失败");
      }
    }, nullptr);
  }
  
  /// @brief 异步写入原始消息
  void AsyncWriteMessages(const std::string& msgid, const std::shared_ptr<RdKafka::Message> message) {
    const char* query = "INSERT INTO ychat.messages (msgid, data) VALUES (?, ?);";
    std::unique_ptr<CassStatement, void(*)(CassStatement*)> statement(cass_statement_new(query, 2), &cass_statement_free);
    cass_statement_bind_string(statement.get(), 0, msgid.c_str());
    cass_statement_bind_bytes(statement.get(), 1, reinterpret_cast<const cass_byte_t*>(message->payload()), message->len());
    // 异步写入
    auto session = Acquire();
	std::unique_ptr<CassFuture, void(*)(CassFuture*)> future(cass_session_execute(session.get(), statement.get()), &cass_future_free);
	Release(std::move(session));
    // 设置写入完成的回调
    cass_future_set_callback(future.get(), [](CassFuture* future, void* data){
      if (cass_future_error_code(future) == CASS_OK) [[likely]] {
        spdlog::info("写入ychat.messages成功");
	  } else {
        spdlog::error("写入ychat.messages失败");
      }
    }, nullptr);
  }

 private:
  std::unique_ptr<CassCluster, void(*)(CassCluster*)> cluster_;
};

// Push节点异步Push
class PushClient : public Singleton<PushClient> {
  friend class Singleton<PushClient>;
 public:
  PushClient() {
	auto channel = grpc::CreateChannel(FLAGS_push_server_address, grpc::InsecureChannelCredentials());
	stub_ = push::PushService::NewStub(channel);
    async_rpc_complete_thread_ = std::make_unique<std::thread>([&](){AsyncRpcComplete();});
  }

  ~PushClient() {
    if (async_rpc_complete_thread_->joinable()) {
  	  async_rpc_complete_thread_->join();
  	  async_rpc_complete_thread_.reset();
	}
  }

  struct AsyncCallContext {
    msg::PushReq  req;
    msg::PushResp resp;
    grpc::ClientContext grpc_context;
    grpc::Status status;
    std::unique_ptr<grpc::ClientAsyncResponseReader<msg::PushResp>> resp_reader;
  };

  void AsyncPush(const std::shared_ptr<msg::MqMessage> mq_message, uint64_t to_uid, int64_t seq) {
    auto* context = new AsyncCallContext;
    // 请求
    context->req.set_to_uid(to_uid);
    context->req.set_seq(seq);
    context->req.mutable_message()->CopyFrom(*mq_message);

	context->resp_reader = stub_->PrepareAsyncPush(&context->grpc_context, context->req, &cq_);
	context->resp_reader->StartCall();
	context->resp_reader->Finish(&context->resp, &context->status, (void*)context);
  }

 private:
  void AsyncRpcComplete() {
    void* got_tag;
    bool ok = false;
	while (cq_.Next(&got_tag, &ok)) {
	  auto* call_context = static_cast<AsyncCallContext*>(got_tag);
	  GPR_ASSERT(ok); // 必须真的完成
	  if (call_context->status.ok()) {
         spdlog::info("推送成功");
		 transfer::Consumer::Instance()->GetThreadPool().Enqueue([&](){
		   CassSeessionPool::Instance()->OnPushSuccess(call_context->req.to_uid(), call_context->req.seq()); // 推送成功
		 });
	  } else {
        spdlog::debug("推送失败");
	  }
	  delete call_context;
	}
  }

 private:
  std::unique_ptr<push::PushService::Stub> stub_; // * 可以做连接池
  // 异步任务
  std::unique_ptr<std::thread> async_rpc_complete_thread_;
  grpc::CompletionQueue cq_;
};

// 消费到消息时候的回调
void CassandraHandle(std::shared_ptr<RdKafka::Message> message) {
  spdlog::debug("消费到一条消息");
  bool is_send_to_group = false;

  std::shared_ptr<msg::MqMessage> mq_message = std::make_shared<msg::MqMessage>();
  if (!mq_message->ParseFromArray(message->payload(), message->len())) {
    spdlog::error("反序列化失败");
    return;
  }
  if (mq_message->handle_result().msgid() != *message->key()) {
    spdlog::error("key错误, `{}` != `{}`", mq_message->handle_result().msgid(), *message->key());
    return;
  }
  if (mq_message->origin_req().type() == msg::ProtoReqType::TypeGroupMsg ||
      mq_message->origin_req().type() == msg::ProtoReqType::TypeGroupCtrl) {
    is_send_to_group = true;
  }

  // * 用户发件箱，这里不做持久化存储

  // 写入msg
  CassSeessionPool::Instance()->AsyncWriteMessages(mq_message->handle_result().msgid(), message);
  message.reset();

  if (is_send_to_group) { // 群聊写扩散推送
    std::vector<unsigned long> uids;
    auto gid = mq_message->origin_req().to_id();
    auto sendtime = mq_message->handle_result().timestamp();
    Poco::Data::Statement statement = mysql::CreateStatement();
    // 查询发送这条数据时的群成员
    try {
      statement << "SELECT user_id FROM group_member WHERE join_time <= ? AND group_id = ?",
                    Poco::Data::Keywords::into(uids),
                    Poco::Data::Keywords::use(sendtime),
                    Poco::Data::Keywords::use(gid),
                    Poco::Data::Keywords::now;
    } catch (const Poco::Data::DataException& e) {
      spdlog::error("查询群成员失败: {}", e.what());
      return;
    }
    // 增加对应的seq
    for (auto to_uid : uids) {
      transfer::Consumer::Instance()->GetThreadPool().Enqueue([mq_message, to_uid](){
        auto seq = redis::redis_pool->hincrby(fmt::format("{}{}", FLAGS_redis_user_prefix, to_uid), "seq", 1);
        // 异步写入用户收件箱
        CassSeessionPool::Instance()->AsyncWriteInbox(mq_message->handle_result().msgid(), to_uid, seq);
        // 异步推送
	    PushClient::Instance()->AsyncPush(mq_message, to_uid, seq);
      });
    }
  } else {
    auto to_uid = mq_message->origin_req().to_id();
    auto seq = redis::redis_pool->hincrby(fmt::format("{}{}", FLAGS_redis_user_prefix, to_uid), "seq", 1);
    // 异步写入用户收件箱
	CassSeessionPool::Instance()->AsyncWriteInbox(mq_message->handle_result().msgid(), to_uid, seq);
    // 异步推送
	PushClient::Instance()->AsyncPush(mq_message, to_uid, seq);
  }
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  spdlog::set_level(spdlog::level::debug);
  mysql::InitMysql(FLAGS_mysql_conn);
  redis::Init(FLAGS_redis_host);
  CassSeessionPool::Instance();

  if (!transfer::Consumer::Instance()->Init(FLAGS_groupid, FLAGS_topic)) {
	spdlog::error("init kafka consumer error");
	return 1;
  }
  transfer::Consumer::Instance()->SetHandler(&CassandraHandle);
  transfer::Consumer::Instance()->Run();
  return 0;
}