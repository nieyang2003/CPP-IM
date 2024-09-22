#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <gflags/gflags.h>
#include <string>
#ifdef SPDLOG_ACTIVE_LEVEL
#undef SPDLOG_ACTIVE_LEVEL
#endif
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

DECLARE_int32(log_level);

inline void InitSpdGflag(int argc, char** argv, const std::string_view& mod_name) {
  assert(!mod_name.empty());
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  auto old_logger = spdlog::default_logger();
  // 创建 sinks，用于控制台和文件的输出
  if ((spdlog::level::level_enum)FLAGS_log_level > spdlog::level::debug) {
    auto daily_logger = spdlog::daily_logger_mt("daily_logger", fmt::format("logs/{}.log", mod_name));
    spdlog::set_default_logger(daily_logger);
  }
  // 设置 logger 和 level
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");
  spdlog::set_level((spdlog::level::level_enum)FLAGS_log_level);
  spdlog::flush_every(std::chrono::seconds(5)); // 定时刷新日志缓冲区
  spdlog::info("init spdlog and gflags success");
  spdlog::default_logger()->flush();
}