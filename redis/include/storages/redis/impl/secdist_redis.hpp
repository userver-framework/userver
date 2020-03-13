#pragma once

#include <chrono>
#include <string>
#include <vector>

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
  std::string password;
};

}  // namespace secdist
