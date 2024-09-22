#pragma once
#include <string>
#include <gflags/gflags.h>
#include <optional>
#include <sw/redis++/redis++.h>
#include "../include/singleton.h"
#include "../include/connection_pool.h"

namespace varify {

class RedisPool : protected ConnectionPool<sw::redis::Redis>, public Singleton<RedisPool> {
  friend class Singleton<RedisPool>;
 public:
  using Client = sw::redis::Redis;

  std::optional<std::string> TryGetKey(const std::string& key);
  bool SetEx(const std::string& key, const std::string& value, int seconds);

 protected:
  RedisPool();
  std::unique_ptr<Client> CreateConnection() override;

};

}