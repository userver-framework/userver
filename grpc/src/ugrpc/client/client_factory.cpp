#include <userver/ugrpc/client/client_factory.hpp>

#include <userver/engine/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

ClientFactory::ClientFactory(
    engine::TaskProcessor& channel_task_processor, grpc::CompletionQueue& queue,
    std::shared_ptr<grpc::ChannelCredentials> credentials,
    const grpc::ChannelArguments& channel_args,
    utils::statistics::Storage& statistics_storage)
    : channel_task_processor_(channel_task_processor),
      queue_(queue),
      channel_cache_(std::move(credentials), channel_args),
      client_statistics_storage_(statistics_storage) {}

impl::ChannelCache::Token ClientFactory::GetChannel(
    const std::string& endpoint) {
  // Spawn a blocking task creating a gRPC channel
  // This is third party code, no use of span inside it
  return engine::AsyncNoSpan(channel_task_processor_,
                             [&] { return channel_cache_.Get(endpoint); })
      .Get();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
