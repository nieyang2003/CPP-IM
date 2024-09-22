#include "redis_pool.h"
#include <spdlog/spdlog.h>
#include <chrono>

namespace {

DEFINE_uint32(max_redis_clients, 4, "redis最大cli数量");
DEFINE_string(redis_url, "tcp://127.0.0.1:6379", "redis地址");
// also see gate/dao/redis/redis.go
DEFINE_string(key_prefix, "ychat::email::", "key前缀");

std::string GenKey(const std::string& key) {
  return fmt::format("{}{}", FLAGS_key_prefix, key);
}

} // namespace

std::optional<std::string> varify::RedisPool::TryGetKey(const std::string &key) {
  auto client = Acquire();
  auto result = client->get(GenKey(key));
  Release(std::move(client));
  if (result.has_value()) {
	return result.value();
  } else {
	return std::nullopt;
  }
}

bool varify::RedisPool::SetEx(const std::string &key, const std::string& value, int seconds) {
  std::unique_ptr<Client> client;

  client = Acquire();
  client->setex(GenKey(key), std::chrono::seconds(seconds), value);
  Release(std::move(client));
  return true;
}

varify::RedisPool::RedisPool() {
  for (size_t i = 0; i < FLAGS_max_redis_clients; ++i) {
  	pool_.push(std::move(CreateConnection()));
  }
}

std::unique_ptr<varify::RedisPool::Client> varify::RedisPool::CreateConnection() {
	// 创建客户端并连接到 Redis 服务器
	auto client = std::make_unique<Client>(FLAGS_redis_url);
	if (!client) {
	  spdlog::error("创建redis客户端失败");
      exit(1);
	}
	// 发送 PING 命令，测试连接
	auto pong = client->ping();
	spdlog::debug("{}", pong);
	return client;
}
