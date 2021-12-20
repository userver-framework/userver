#include <userver/ugrpc/client/client_factory.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

ClientFactory::ClientFactory(
    engine::TaskProcessor& channel_task_processor, grpc::CompletionQueue& queue,
    std::shared_ptr<grpc::ChannelCredentials> credentials,
    const grpc::ChannelArguments& channel_args)
    : channel_task_processor_(channel_task_processor),
      queue_(queue),
      channel_cache_(std::move(credentials), channel_args) {}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
