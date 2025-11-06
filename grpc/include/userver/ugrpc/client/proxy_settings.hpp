#pragma once

#include <string>
#include <unordered_set>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

struct ProxySettings {
    /// Proxy server address
    /// No proxy if address is empty
    std::string proxy_address;

    /// List of addresses that should be accessed directly, bypassing proxy server
    std::unordered_set<std::string> no_proxy_targets;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
