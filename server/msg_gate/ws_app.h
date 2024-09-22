// uws服务

#pragma once
#include <string>
#include <uWebSockets/App.h>
#include <vector>
#include <thread>
#include <memory>

namespace msg::msg_gate {

void RunWsApp(std::vector<std::unique_ptr<std::thread>> *threads);

} // namespace msg
