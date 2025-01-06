#pragma once

/// @file userver/ugrpc/client/client_factory_settings.hpp
/// @brief @copybrief ugrpc::client::ClientFactorySettings

#include <memory>
#include <string>
#include <unordered_map>

#include <grpcpp/security/credentials.h>
#include <grpcpp/support/channel_arguments.h>

#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

// full rpc name -> count of channels
using DedicatedMethodsConfig = std::unordered_map<std::string, std::size_t>;

/// Settings relating to the ClientFactory
struct ClientFactorySettings final {
    /// gRPC channel credentials, none by default
    std::shared_ptr<grpc::ChannelCredentials> credentials{grpc::InsecureChannelCredentials()};

    /// gRPC channel credentials by client_name. If not set, default `credentials`
    /// is used instead.
    std::unordered_map<std::string, std::shared_ptr<grpc::ChannelCredentials>> client_credentials{};

    /// Optional grpc-core channel args
    /// @see https://grpc.github.io/grpc/core/group__grpc__arg__keys.html
    grpc::ChannelArguments channel_args{};

    /// The logging level override for the internal grpcpp library. Must be either
    /// `kDebug`, `kInfo` or `kError`.
    logging::Level native_log_level{logging::Level::kError};

    /// Number of underlying channels that will be created for every client
    /// in this factory.
    std::size_t channel_count{1};
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
