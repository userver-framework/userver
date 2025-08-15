#pragma once

#include <optional>
#include <string>

#include <grpcpp/support/channel_arguments.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

grpc::ChannelArguments
BuildChannelArguments(const grpc::ChannelArguments& channel_args, const std::optional<std::string>& service_config);

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
