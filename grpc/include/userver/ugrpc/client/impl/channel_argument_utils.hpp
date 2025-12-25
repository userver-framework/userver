#pragma once

#include <optional>
#include <string>

#include <grpcpp/support/channel_arguments.h>

#include <userver/ugrpc/client/auth_type.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

grpc::ChannelArguments BuildChannelArguments(
    const grpc::ChannelArguments& channel_args,
    const std::optional<std::string>& service_config
);

void SetHttpProxy(
    std::string& target,
    grpc::ChannelArguments& channel_args,
    AuthType auth_type,
    const std::string& proxy_address
);

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
