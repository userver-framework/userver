#pragma once

/// @file userver/ugrpc/client/client_factory.hpp
/// @brief @copybrief ugrpc::client::ClientFactory

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include <grpcpp/completion_queue.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/support/channel_arguments.h>

#include <userver/dynamic_config/source.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/logging/level.hpp>
#include <userver/storages/secdist/secdist.hpp>
#include <userver/testsuite/grpc_control.hpp>
#include <userver/yaml_config/fwd.hpp>

#include <userver/ugrpc/client/impl/channel_cache.hpp>
#include <userver/ugrpc/client/impl/client_data.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {
class StatisticsStorage;
}  // namespace ugrpc::impl

namespace ugrpc::client {

/// Settings relating to the ClientFactory
struct ClientFactorySettings final {
  /// gRPC channel credentials, none by default
  std::shared_ptr<grpc::ChannelCredentials> credentials{
      grpc::InsecureChannelCredentials()};

  /// gRPC channel credentials by client_name. If not set, default `credentials`
  /// is used instead.
  std::unordered_map<std::string, std::shared_ptr<grpc::ChannelCredentials>>
      client_credentials{};

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

/// @ingroup userver_clients
///
/// @brief Creates gRPC clients.
///
/// Typically obtained from ugrpc::client::ClientFactoryComponent.
/// In tests and benchmarks, obtained from ugrpc::tests::ServiceBase and
/// friends.
///
/// Has a minimal built-in channel cache:
/// as long as a channel to the same endpoint is used somewhere, the same
/// channel is given out.
class ClientFactory final {
 public:
  /// @brief Make a client of the specified code-generated type.
  ///
  /// @tparam Client the type of the code-generated usrv grpc client.
  /// @param client_name the name of the client, for diagnostics, credentials
  /// and middlewares.
  /// @param endpoint the URI to connect to, e.g. `http://my.domain.com:8080`.
  /// Should not include any HTTP path, just schema, domain name and port. Unix
  /// sockets are also supported. For details, see:
  /// https://grpc.github.io/grpc/cpp/md_doc_naming.html
  template <typename Client>
  Client MakeClient(const std::string& client_name,
                    const std::string& endpoint);

  /// @cond
  // For internal use only.
  ClientFactory(ClientFactorySettings&& settings,
                engine::TaskProcessor& channel_task_processor,
                MiddlewareFactories mws, grpc::CompletionQueue& queue,
                ugrpc::impl::StatisticsStorage& statistics_storage,
                testsuite::GrpcControl& testsuite_grpc,
                dynamic_config::Source source);
  /// @endcond

 private:
  impl::ChannelCache::Token GetChannel(const std::string& client_name,
                                       const std::string& endpoint);

  engine::TaskProcessor& channel_task_processor_;
  MiddlewareFactories mws_;
  grpc::CompletionQueue& queue_;
  impl::ChannelCache channel_cache_;
  std::unordered_map<std::string, impl::ChannelCache> client_channel_cache_;
  ugrpc::impl::StatisticsStorage& client_statistics_storage_;
  const dynamic_config::Source config_source_;
  testsuite::GrpcControl& testsuite_grpc_;
};

template <typename Client>
Client ClientFactory::MakeClient(const std::string& client_name,
                                 const std::string& endpoint) {
  return Client(impl::ClientParams{
      client_name,
      endpoint,
      impl::InstantiateMiddlewares(mws_, client_name),
      queue_,
      client_statistics_storage_,
      GetChannel(client_name, endpoint),
      config_source_,
      testsuite_grpc_,
  });
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
