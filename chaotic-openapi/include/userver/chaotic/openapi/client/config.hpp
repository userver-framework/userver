#pragma once

#include <chrono>
#include <string>

#include <userver/chaotic/openapi/client/digest_auth.hpp>

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

    DigestAuthCredentialsByScheme digest_auth_credentials{};
};

Config ParseConfig(const components::ComponentConfig& config, std::string_view base_url);

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
