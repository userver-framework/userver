#pragma once

#include <chrono>
#include <string>

namespace samples::pg {

using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

struct UserDbInfo {
  std::string username;
  std::string nonce;
  TimePoint timestamp;
  int nonce_count{};
  std::string ha1;
};

}  // namespace samples::pg
