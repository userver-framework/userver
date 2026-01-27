#include <userver/ugrpc/client/impl/channel_argument_utils.hpp>

#include <grpc/grpc.h>

#include <userver/logging/log.hpp>

#include <userver/ugrpc/impl/to_string.hpp>

#include <dynamic_config/variables/EGRESS_GRPC_PROXY_ENABLED.hpp>
#include <dynamic_config/variables/EGRESS_NO_PROXY_TARGETS.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

grpc::ChannelArguments BuildChannelArguments(
    const grpc::ChannelArguments& channel_args,
    const std::optional<std::string>& service_config
) {
    if (!service_config.has_value()) {
        return channel_args;
    }

    LOG_INFO() << "Building ChannelArguments, ServiceConfig: " << *service_config;
    auto effective_channel_args{channel_args};
    effective_channel_args.SetServiceConfigJSON(ugrpc::impl::ToGrpcString(*service_config));
    return effective_channel_args;
}

void SetHttpProxy(
    std::string& target,
    grpc::ChannelArguments& channel_args,
    AuthType auth_type,
    const std::string& proxy_address
) {
    switch (auth_type) {
        case AuthType::kInsecure:
            channel_args.SetString(GRPC_ARG_DEFAULT_AUTHORITY, ugrpc::impl::ToGrpcString(target));
            target = proxy_address;
            break;
        case AuthType::kSsl:
            channel_args
                .SetString(GRPC_ARG_HTTP_PROXY, ugrpc::impl::ToGrpcString(fmt::format("http://{}", proxy_address)));
            break;
    }
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
