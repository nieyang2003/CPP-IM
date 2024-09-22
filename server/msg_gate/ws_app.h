// uws服务

#pragma once
#include <string>
#include <uWebSockets/App.h>
#include <vector>
#include <thread>
#include <memory>
#include "include/thread_pool.h"

namespace msg::msg_gate {

/// @brief 阻塞运行ws程序
/// @param pool 线程池
/// @param redis redis连接池
/// @param gate_location ws监听地址
void RunWsApp(std::shared_ptr<ThreadPool> pool, const std::string& gate_location);

} // namespace msg
