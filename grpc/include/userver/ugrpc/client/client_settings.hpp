#pragma once

/// @file userver/ugrpc/client/client_settings.hpp
/// @brief @copybrief ugrpc::client::ClientSettings

#include <cstddef>
#include <string>
#include <unordered_map>

#include <userver/dynamic_config/snapshot.hpp>

#include <userver/ugrpc/client/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

// rpc method name -> count of channels
using DedicatedMethodsConfig = std::unordered_map<std::string, std::size_t>;

/// Settings relating to creation of a code-generated client
struct ClientSettings final {
    /// **(Required)**
    /// The name of the client, for diagnostics, credentials and middlewares.
    std::string client_name;

    /// **(Required)**
    /// The URI to connect to, e.g. `http://my.domain.com:8080`.
    /// Should not include any HTTP path, just schema, domain name and port. Unix
    /// sockets are also supported. For details, see:
    /// https://grpc.github.io/grpc/cpp/md_doc_naming.html
    std::string endpoint;

    /// **(Optional)**
    /// The name of the QOS
    /// @ref scripts/docs/en/userver/dynamic_config.md "dynamic config"
    /// that will be applied automatically to every RPC.
    ///
    /// Timeout from QOS config is ignored if:
    ///
    /// * an explicit `qos` parameter is specified at RPC creation, or
    /// * deadline is specified in the `client_context` passed at RPC creation.
    ///
    /// ## Client QOS config definition sample
    ///
    /// @snippet grpc/tests/tests/unit_test_client_qos.hpp  qos config key
    const dynamic_config::Key<ClientQos>* client_qos{nullptr};

    /// **(Optional)**
    /// Dedicated high-load methods that have separate channels
    DedicatedMethodsConfig dedicated_methods_config{};
};

std::size_t GetMethodChannelCount(const DedicatedMethodsConfig& dedicated_methods_config, std::string_view method_name);

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
