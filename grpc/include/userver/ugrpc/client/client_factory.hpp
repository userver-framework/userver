#pragma once

/// @file userver/ugrpc/client/client_factory.hpp
/// @brief @copybrief ugrpc::client::ClientFactory

#include <cstddef>

#include <grpcpp/completion_queue.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/support/channel_arguments.h>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/logging/level.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/yaml_config/fwd.hpp>

#include <userver/ugrpc/client/impl/channel_cache.hpp>
#include <userver/ugrpc/impl/statistics_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// Settings relating to the ClientFactory
struct ClientFactoryConfig {
  /// gRPC channel credentials, none by default
  std::shared_ptr<grpc::ChannelCredentials> credentials{
      grpc::InsecureChannelCredentials()};

  /// Optional grpc-core channel args
  /// @see https://grpc.github.io/grpc/core/group__grpc__arg__keys.html
  grpc::ChannelArguments channel_args{};

  /// The logging level override for the internal grpcpp library. Must be either
  /// `kDebug`, `kInfo` or `kError`.
  logging::Level native_log_level{logging::Level::kError};

  /// Number of underlying channels that will be created for every client
  /// in this factory.
  std::size_t channel_count{1};
};

ClientFactoryConfig Parse(const yaml_config::YamlConfig& value,
                          formats::parse::To<ClientFactoryConfig>);

/// @brief Creates generated gRPC clients. Has a minimal built-in channel cache:
/// as long as a channel to the same endpoint is used somewhere, the same
/// channel is given out.
class ClientFactory final {
 public:
  ClientFactory(ClientFactoryConfig&& config,
                engine::TaskProcessor& channel_task_processor,
                grpc::CompletionQueue& queue,
                utils::statistics::Storage& statistics_storage);

  template <typename Client>
  Client MakeClient(const std::string& endpoint);

 private:
  impl::ChannelCache::Token GetChannel(const std::string& endpoint);

  engine::TaskProcessor& channel_task_processor_;
  grpc::CompletionQueue& queue_;
  impl::ChannelCache channel_cache_;
  ugrpc::impl::StatisticsStorage client_statistics_storage_;
};

template <typename Client>
Client ClientFactory::MakeClient(const std::string& endpoint) {
  auto& statistics =
      client_statistics_storage_.GetServiceStatistics(Client::GetMetadata());
  return Client(GetChannel(endpoint), queue_, statistics);
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
