#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include <grpcpp/support/channel_arguments.h>

#include <userver/formats/json/value.hpp>

#include <userver/ugrpc/client/fwd.hpp>
#include <userver/ugrpc/impl/static_service_metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class ServiceConfigBuilder final {
public:
    struct PreparedMethodConfigs {
        // method_id -> method_config
        std::unordered_map<std::size_t, formats::json::Value> method_configs;
        std::optional<formats::json::Value> default_method_config;
    };

    ServiceConfigBuilder(
        const ugrpc::impl::StaticServiceMetadata& metadata,
        const std::optional<std::string>& static_service_config
    );

    ServiceConfigBuilder(ServiceConfigBuilder&&) noexcept = default;
    ServiceConfigBuilder(const ServiceConfigBuilder&) = delete;
    ServiceConfigBuilder& operator=(ServiceConfigBuilder&&) = delete;
    ServiceConfigBuilder& operator=(const ServiceConfigBuilder&) = delete;

    formats::json::Value Build(const ClientQos& client_qos) const;

private:
    formats::json::Value BuildMethodConfigArray(const ClientQos& client_qos) const;

    ugrpc::impl::StaticServiceMetadata metadata_;

    formats::json::Value static_service_config_;

    PreparedMethodConfigs prepared_method_configs_;
};

class ChannelArgumentsBuilder final {
public:
    ChannelArgumentsBuilder(
        const grpc::ChannelArguments& channel_args,
        const std::optional<std::string>& static_service_config,
        const ugrpc::impl::StaticServiceMetadata& metadata
    );

    ChannelArgumentsBuilder(ChannelArgumentsBuilder&&) noexcept = default;
    ChannelArgumentsBuilder(const ChannelArgumentsBuilder&) = delete;
    ChannelArgumentsBuilder& operator=(ChannelArgumentsBuilder&&) = delete;
    ChannelArgumentsBuilder& operator=(const ChannelArgumentsBuilder&) = delete;

    grpc::ChannelArguments Build(const ClientQos& client_qos) const;

private:
    const grpc::ChannelArguments& channel_args_;

    ServiceConfigBuilder service_config_builder_;
};

grpc::ChannelArguments
BuildChannelArguments(const grpc::ChannelArguments& channel_args, const std::optional<std::string>& service_config);

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
