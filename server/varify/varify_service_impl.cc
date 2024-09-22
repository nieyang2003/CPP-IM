#include "varify_service_impl.h"
#include "smtp.h"
#include "redis_pool.h"
#include <random>
#include <spdlog/spdlog.h>

namespace {

std::string GenVarifyCode() {
  std::random_device rd;  // 产生一个真正的随机数
  std::mt19937 gen(rd()); // 使用Mersenne Twister算法
  std::uniform_int_distribution<> dis(100000, 999999); // 生成100000到999999之间的随机数
  return std::to_string(dis(gen));
}

} // namesapce

namespace varify {

grpc::Status VarifyServiceImpl::GetVarifyCode(grpc::ServerContext *context,
  const GetVarifyCodeRequest *request, GetVarifyCodeResponse *response) {
  spdlog::info("调用者：{}", context->peer());

  // 查找redis key
  if (auto value = RedisPool::Instance()->TryGetKey(request->email()); value) {
	spdlog::info("缓存命中：{} - {}", request->email(), *value);
	response->set_code(*value);
	return grpc::Status::OK;
  }
  // 未找到，是否发送
  if (!request->if_send()) {
	return grpc::Status(grpc::NOT_FOUND, "未找到此邮箱");
  }

  auto code = GenVarifyCode();
  if (!SmtpPoll::Instance()->Send(request->email(), "验证码", code)) {
	spdlog::error("发送失败：{}", request->email());

	return grpc::Status(grpc::INTERNAL, "发送失败");
  } else {
	spdlog::info("发送成功：{} - {}", request->email(), code);

	// 保存验证码，并设置180s超时时间
	RedisPool::Instance()->SetEx(request->email(), code, 180);

	response->set_code(code);
	return grpc::Status::OK;
  }
}

} // namespace varify