#pragma once
#include <string>
#include <chrono>
#include <random>
#include <spdlog/spdlog.h>
#include "proto/msg.pb.h"

inline std::string GenRandString(int length) {
  static const std::string kCharset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

  std::string result;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, kCharset.size() - 1);

  for (int i = 0; i < length; ++i) {
    result += kCharset[dis(gen)];
  }

  return result;
}

/// @brief 生成msgid和1970-1-1以来的ms数
inline msg::ProtoResp GenProtoResp(uint64_t uid) {
  auto ms = std::chrono::duration_cast< std::chrono::milliseconds >(
    std::chrono::system_clock::now().time_since_epoch()
  ).count();
  msg::ProtoResp resp;
  resp.set_msgid(fmt::format("{}_{}_{}", ms, uid, GenRandString(8)));
  resp.set_timestamp(ms);
  return resp;
}