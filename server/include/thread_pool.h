#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class ThreadPool {
 public:
  explicit ThreadPool(size_t = std::thread::hardware_concurrency());
  template <class F, class... Args>
  auto Enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>;
  ~ThreadPool();

 private:
  // need to keep track of threads so we can join them
  std::vector<std::thread> workers_;
  // the task queue
  std::queue<std::function<void()>> tasks_;

  // synchronization
  std::mutex queue_mutex_;
  std::condition_variable condition_;
  bool stop_;
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads)
  : stop_(false) {
  for (size_t i = 0; i < threads; ++i)
    workers_.emplace_back([this]() {
      for (;;) {
        std::function<void()> task;
        {
			// 等待
            std::unique_lock<std::mutex> lock(this->queue_mutex_);
            this->condition_.wait(lock,
                                 [this]
                                 { return this->stop_ || !this->tasks_.empty(); });
			// 所有任务处理完毕且关闭
            if (this->stop_ && this->tasks_.empty())
                return;
			// 取出任务
            task = std::move(this->tasks_.front());
            this->tasks_.pop();
        }
        task();
      }
    });
}

// add new work item to the pool
template <class F, class... Args>
auto ThreadPool::Enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type> {
  using return_type = typename std::result_of<F(Args...)>::type;

  auto task = std::make_shared<std::packaged_task<return_type()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));

  std::future<return_type> res = task->get_future();
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    // don't allow enqueueing after stopping the pool
    if (stop_) {
      throw std::runtime_error("enqueue on stopped ThreadPool");
	}
    tasks_.emplace([task]() { (*task)(); });
  }
  condition_.notify_one();
  return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    stop_ = true;
  }
  condition_.notify_all();
  for (std::thread &worker : workers_)
    worker.join();
}

#endif