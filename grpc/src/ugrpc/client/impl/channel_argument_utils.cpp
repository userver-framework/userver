#include <userver/ugrpc/client/impl/channel_argument_utils.hpp>

#include <userver/logging/log.hpp>

#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

grpc::ChannelArguments
BuildChannelArguments(const grpc::ChannelArguments& channel_args, const std::optional<std::string>& service_config) {
    if (!service_config.has_value()) {
        return channel_args;
    }

    LOG_INFO() << "Building ChannelArguments, ServiceConfig: " << *service_config;
    auto effective_channel_args{channel_args};
    effective_channel_args.SetServiceConfigJSON(ugrpc::impl::ToGrpcString(*service_config));
    return effective_channel_args;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
