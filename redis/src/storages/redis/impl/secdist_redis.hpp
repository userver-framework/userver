#pragma once

#include <chrono>
#include <string>
#include <vector>

#include <userver/storages/redis/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace secdist {

struct RedisSettings {
    struct HostPort {
        std::string host;
        int port;

        explicit HostPort(std::string host = {}, int port = 0) noexcept : host(std::move(host)), port(port) {}
    };

    std::vector<std::string> shards;
    std::vector<HostPort> sentinels;
    storages::redis::Password password{std::string()};
    storages::redis::ConnectionSecurity secure_connection{storages::redis::ConnectionSecurity::kNone};
    std::size_t database_index{0};
};

}  // namespace secdist

USERVER_NAMESPACE_END
