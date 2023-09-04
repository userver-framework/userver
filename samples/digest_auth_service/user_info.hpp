#pragma once

#include <chrono>
#include <string>

#include <userver/storages/postgres/io/chrono.hpp>

namespace samples::digest_auth {

struct UserDbInfo {
  std::string username;
  std::string nonce;
  storages::postgres::TimePointTz timestamp;
  int nonce_count{};
  std::string ha1;
};

}  // namespace samples::digest_auth
