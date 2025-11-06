#pragma once

/// @file userver/etcd/settings.hpp
/// @brief etcd client settings

#include <chrono>
#include <string>
#include <vector>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace etcd {

/// @brief Etcd client settigs struct
struct ClientSettings final {
    // Etcd endpoints to which client make HTTP requests
    std::vector<std::string> endpoints;
    // Number of attempts to each endpoint, on failed attempts client randomly moves to another endpoint
    std::uint32_t attempts;
    // Timeout for all HTTP requests to etcd except watch request
    std::chrono::microseconds request_timeout_ms;
    // Timeout for watch HTTP request. It's a stremed request, so it is used also as a connection timeout, so it should
    // not be too short
    std::chrono::microseconds watch_timeout_ms;
};

}  // namespace etcd

namespace formats::parse {

etcd::ClientSettings Parse(const yaml_config::YamlConfig& value, To<etcd::ClientSettings>);

}

USERVER_NAMESPACE_END
