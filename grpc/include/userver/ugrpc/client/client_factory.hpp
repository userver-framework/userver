#pragma once

/// @file userver/ugrpc/client/client_factory.hpp
/// @brief @copybrief ugrpc::client::ClientFactory

#include <grpcpp/completion_queue.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/support/channel_arguments.h>

#include <userver/engine/async.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/ugrpc/client/impl/channel_cache.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @brief Creates generated gRPC clients. Has a minimal built-in channel cache:
/// as long as a channel to the same endpoint is used somewhere, the same
/// channel is given out.
class ClientFactory final {
 public:
  ClientFactory(engine::TaskProcessor& channel_task_processor,
                grpc::CompletionQueue& queue,
                std::shared_ptr<grpc::ChannelCredentials> credentials,
                const grpc::ChannelArguments& channel_args);

  template <typename Client>
  Client MakeClient(const std::string& endpoint);

 private:
  engine::TaskProcessor& channel_task_processor_;
  grpc::CompletionQueue& queue_;
  impl::ChannelCache channel_cache_;
};

template <typename Client>
Client ClientFactory::MakeClient(const std::string& endpoint) {
  // Spawn a blocking task creating a gRPC channel
  // This is third party code, no use of span inside it
  return engine::AsyncNoSpan(
             channel_task_processor_,
             [&] { return Client(channel_cache_.Get(endpoint), queue_); })
      .Get();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
