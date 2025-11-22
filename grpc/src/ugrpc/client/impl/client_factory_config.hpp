#pragma once

#include <grpcpp/security/credentials.h>
#include <grpcpp/support/channel_arguments.h>

#include <userver/yaml_config/yaml_config.hpp>

#include <userver/ugrpc/client/auth_type.hpp>
#include <userver/ugrpc/client/retry_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

/// Settings relating to the ClientFactory
struct ClientFactoryConfig final {
    AuthType auth_type{AuthType::kInsecure};

    grpc::SslCredentialsOptions ssl_credentials_options{};

    /// Retry configuration for outgoing RPCs
    RetryConfig retry_config;

    /// Optional grpc-core channel args
    /// @see https://grpc.github.io/grpc/core/group__grpc__arg__keys.html
    grpc::ChannelArguments channel_args{};

    /// default service config
    /// @see https://github.com/grpc/grpc/blob/master/doc/service_config.md
    std::optional<std::string> default_service_config;

    /// Number of underlying channels that will be created for every client
    /// in this factory.
    std::size_t channel_count{1};
};

ClientFactoryConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<ClientFactoryConfig>);

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
