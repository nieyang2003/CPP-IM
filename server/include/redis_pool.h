#pragma once
#include <sw/redis++/redis++.h>
#include <spdlog/spdlog.h>
#include <memory>

namespace redis {

inline std::shared_ptr<sw::redis::Redis> redis_pool;

inline void Init(const std::string& host, int port = 6379) {
  // redis连接池
  sw::redis::ConnectionPoolOptions pool_opts;
  pool_opts.size = 10;
  pool_opts.wait_timeout = std::chrono::milliseconds(100);
  pool_opts.connection_lifetime = std::chrono::milliseconds(0);
  sw::redis::ConnectionOptions conn_opts;
  conn_opts.keep_alive = true;
  conn_opts.host = host;
  conn_opts.port = port;
  conn_opts.socket_timeout = std::chrono::milliseconds(100);
  try {
    redis_pool = std::make_shared<sw::redis::Redis>(conn_opts, pool_opts);
    // 通过执行简单的 Redis 命令来测试连接
    auto reply = redis_pool->ping();
    spdlog::debug("connected to redis: {}", reply);
  } catch (const sw::redis::Error &e) {
    spdlog::error("failed to connect to redis: {}", e.what());
    throw;
  }
  assert(redis_pool);
}

}