#pragma once

#include <chrono>
#include <string>

namespace samples::digest_auth {

using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

struct UserDbInfo {
  std::string username;
  std::string nonce;
  TimePoint timestamp;
  int nonce_count{};
  std::string ha1;
};

}  // namespace samples::digest_auth
