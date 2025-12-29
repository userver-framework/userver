#include <userver/ugrpc/client/impl/channel_factory.hpp>

#include <grpcpp/create_channel.h>

#include <userver/engine/async.hpp>

#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

ChannelFactory::ChannelFactory(
    engine::TaskProcessor& blocking_task_processor,
    std::shared_ptr<grpc::ChannelCredentials> credentials,
    AuthType auth_type
)
    : blocking_task_processor_(blocking_task_processor),
      credentials_{std::move(credentials)},
      auth_type_(auth_type)
{}

std::shared_ptr<grpc::Channel> ChannelFactory::CreateChannel(
    std::string_view target,
    const grpc::ChannelArguments& channel_args
) const {
    return engine::AsyncNoSpan(
               blocking_task_processor_,
               grpc::CreateCustomChannel,
               ugrpc::impl::ToGrpcString(target),
               std::ref(credentials_),
               std::ref(channel_args)
    )
        .Get();
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
