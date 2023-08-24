#pragma once

#include <chrono>
#include <string>
#include <userver/utest/using_namespace_userver.hpp>

#include <vector>

#include <userver/cache/base_postgres_cache.hpp>
#include <userver/crypto/algorithm.hpp>
#include <userver/server/auth/user_auth_info.hpp>
#include <userver/storages/postgres/io/array_types.hpp>

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
