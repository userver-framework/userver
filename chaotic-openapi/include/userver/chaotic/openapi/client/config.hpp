#pragma once

#include <chrono>
#include <string>

#include <userver/chaotic/openapi/client/auth.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
class ComponentConfig;
}

namespace clients::http {
class Request;
}

namespace chaotic::openapi::client {

struct Config {
    std::string base_url;
    int attempts{1};
    std::chrono::milliseconds timeout{100};

    /// Auth providers by the securitySchemes key, see chaotic::openapi::client::AuthProvider
    AuthProviders auth_providers{};
};

Config ParseConfig(const components::ComponentConfig& config, std::string_view base_url);

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
