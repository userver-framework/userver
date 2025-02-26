#include <userver/ugrpc/client/impl/channel_factory.hpp>

#include <grpcpp/create_channel.h>

#include <userver/engine/async.hpp>

#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

ChannelFactory::ChannelFactory(
    engine::TaskProcessor& blocking_task_processor,
    std::string endpoint,
    std::shared_ptr<grpc::ChannelCredentials> credentials
)
    : blocking_task_processor_(blocking_task_processor),
      endpoint_{ugrpc::impl::ToGrpcString(std::move(endpoint))},
      credentials_{std::move(credentials)} {}

std::shared_ptr<grpc::Channel> ChannelFactory::CreateChannel(const grpc::ChannelArguments& channel_args) const {
    return engine::AsyncNoSpan(
               blocking_task_processor_,
               grpc::CreateCustomChannel,
               std::ref(endpoint_),
               std::ref(credentials_),
               std::ref(channel_args)
    )
        .Get();
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
