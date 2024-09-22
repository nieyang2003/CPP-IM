#pragma once
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

template<typename T>
class ConnectionPool {
 public:
  ConnectionPool() = default;

  virtual ~ConnectionPool() {
    Shutdown();
  }

  std::unique_ptr<T> Acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this]() { return is_stop_ || !pool_.empty(); });
    if (is_stop_.load()) {
	  return nullptr;
	}
    auto conn = std::move(pool_.front());
    pool_.pop();
    return conn;
  }

  void Release(std::unique_ptr<T> conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    pool_.push(std::move(conn));
    cond_.notify_one();
  }

  void Shutdown() {
	if (is_stop_.load()) return;
    std::lock_guard<std::mutex> lock(mutex_);
	is_stop_.store(true);
	cond_.notify_all();
    while (!pool_.empty()) {
      pool_.pop();
    }
    cond_.notify_all();
  }

 protected:
  virtual std::unique_ptr<T> CreateConnection() = 0;

  std::queue<std::unique_ptr<T>> pool_;
  std::mutex mutex_;
  std::condition_variable cond_;
  std::atomic_bool is_stop_ { false };
};