#pragma once

#include <chrono>
#include <string>
#include <vector>

#include <userver/storages/redis/impl/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace secdist {

struct RedisSettings {
  struct HostPort {
    std::string host;
    int port;

    explicit HostPort(std::string host = {}, int port = 0)
        : host(std::move(host)), port(port) {}
  };

  std::vector<std::string> shards;
  std::vector<HostPort> sentinels;
  redis::Password password{std::string()};
  redis::ConnectionSecurity secure_connection{redis::ConnectionSecurity::kNone};
};

}  // namespace secdist

USERVER_NAMESPACE_END
