#pragma once
#include <string>
#include <unordered_set>
#include <memory>
#include <spdlog/spdlog.h>

inline std::vector<std::string_view> Split(std::string_view s, std::string_view delim, bool keep_empty) {
  std::vector<std::string_view> splited;
  if (s.empty()) {
	return splited;
  }
  if (delim.empty()) [[unlikely]] {
    spdlog::error("请提供至少一个令牌");
	exit(1);
  }

  auto cur = s;
  while (true) {
    auto pos = cur.find(delim);
	if (pos != 0 || keep_empty) {
	  splited.push_back(cur.substr(0, pos));
	}
	if (pos == std::string_view::npos) {
	  break;
	}
	cur = cur.substr(pos + delim.size());
	if (cur.empty()) {
	  if (keep_empty) {
		splited.push_back("");
	  }
	  break;
	}
  }

  return splited;
}

inline std::vector<std::string_view> Split(std::string_view s, char delim, bool keep_empty) {
  return Split(s, std::string_view(&delim, 1), keep_empty);
}

/// @brief token验证器
class TokenVerifier {
 public:
  TokenVerifier() = default;

  explicit TokenVerifier(std::unordered_set<std::string> recognized_tokens)
    : recognized_tokens_(std::move(recognized_tokens)) {
	if (recognized_tokens_.empty()) [[unlikely]] {
  	  spdlog::error("请提供至少一个令牌");
    }
    if (recognized_tokens_.count("") != 0) [[unlikely]] {
  	  spdlog::error("存在空令牌，可能导致安全漏洞");
    }
  }
  
  bool Verify(const std::string& token) const noexcept {
	return recognized_tokens_.count(token) != 0;
  }

 private:
  std::unordered_set<std::string> recognized_tokens_;
};

/// @brief 由字符串创建一个token验证器
/// @param str 
/// @return 
inline std::unique_ptr<TokenVerifier> MakeTokenVerifier(const std::string& str) {
  if (str.empty()) [[unlikely]] {
    spdlog::error("存在空令牌，可能导致安全漏洞");
	exit(1);
  }
  auto tokens = Split(str, ',', true /* keep_empty */);

  std::unordered_set<std::string> translated;
  for (auto&& e : tokens) {
    translated.insert(std::string(e));
  }
  return std::make_unique<TokenVerifier>(translated);
}