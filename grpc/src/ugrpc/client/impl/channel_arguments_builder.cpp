#include <userver/ugrpc/client/impl/channel_arguments_builder.hpp>

#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

ChannelArgumentsBuilder::ChannelArgumentsBuilder(
    const grpc::ChannelArguments& channel_args,
    const std::optional<std::string>& default_service_config
)
    : default_channel_args_{channel_args} {
    if (default_service_config.has_value()) {
        default_channel_args_.SetServiceConfigJSON(ugrpc::impl::ToGrpcString(*default_service_config));
    }
}

grpc::ChannelArguments ChannelArgumentsBuilder::Build(const ClientQos& /*client_qos*/) const {
    /// TBD
    return default_channel_args_;
}

grpc::ChannelArguments ChannelArgumentsBuilder::Build() const { return default_channel_args_; }

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
