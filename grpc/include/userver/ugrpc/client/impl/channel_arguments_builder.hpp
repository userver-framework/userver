#pragma once

#include <optional>

#include <grpcpp/support/channel_arguments.h>

#include <userver/ugrpc/client/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class ChannelArgumentsBuilder final {
public:
    ChannelArgumentsBuilder(
        const grpc::ChannelArguments& channel_args,
        const std::optional<std::string>& default_service_config
    );

    ChannelArgumentsBuilder(ChannelArgumentsBuilder&&) = default;
    ChannelArgumentsBuilder(const ChannelArgumentsBuilder&) = delete;
    ChannelArgumentsBuilder& operator=(ChannelArgumentsBuilder&&) = delete;
    ChannelArgumentsBuilder& operator=(const ChannelArgumentsBuilder&) = delete;

    grpc::ChannelArguments Build(const ClientQos& client_qos) const;
    grpc::ChannelArguments Build() const;

private:
    grpc::ChannelArguments default_channel_args_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
