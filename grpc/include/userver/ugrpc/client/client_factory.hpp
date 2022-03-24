#pragma once

/// @file userver/ugrpc/client/client_factory.hpp
/// @brief @copybrief ugrpc::client::ClientFactory

#include <grpcpp/completion_queue.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/support/channel_arguments.h>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/statistics/fwd.hpp>

#include <userver/ugrpc/client/impl/channel_cache.hpp>
#include <userver/ugrpc/client/impl/statistics_storage.hpp>

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
                const grpc::ChannelArguments& channel_args,
                utils::statistics::Storage& statistics_storage);

  template <typename Client>
  Client MakeClient(const std::string& endpoint);

 private:
  impl::ChannelCache::Token GetChannel(const std::string& endpoint);

  engine::TaskProcessor& channel_task_processor_;
  grpc::CompletionQueue& queue_;
  impl::ChannelCache channel_cache_;
  impl::StatisticsStorage client_statistics_storage_;
};

template <typename Client>
Client ClientFactory::MakeClient(const std::string& endpoint) {
  auto& statistics =
      client_statistics_storage_.GetServiceStatistics(Client::GetMetadata());
  return Client(GetChannel(endpoint), queue_, statistics);
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
