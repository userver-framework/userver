#include <userver/ugrpc/client/client_factory.hpp>

#include <optional>
#include <stdexcept>

#include <fmt/format.h>

#include <userver/engine/async.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/utils/algo.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <ugrpc/client/impl/client_factory_config.hpp>
#include <ugrpc/impl/logging.hpp>
#include <ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

ClientFactory::ClientFactory(ClientFactorySettings&& settings,
                             engine::TaskProcessor& channel_task_processor,
                             MiddlewareFactories mws,
                             grpc::CompletionQueue& queue,
                             ugrpc::impl::StatisticsStorage& statistics_storage,
                             testsuite::GrpcControl& testsuite_grpc,
                             dynamic_config::Source source)
    : channel_task_processor_(channel_task_processor),
      mws_(mws),
      queue_(queue),
      channel_cache_(testsuite_grpc.IsTlsEnabled()
                         ? settings.credentials
                         : grpc::InsecureChannelCredentials(),
                     settings.channel_args, settings.channel_count),
      client_statistics_storage_(statistics_storage),
      config_source_(source),
      testsuite_grpc_(testsuite_grpc) {
  ugrpc::impl::SetupNativeLogging();
  ugrpc::impl::UpdateNativeLogLevel(settings.native_log_level);

  for (auto& [client_name, creds] : settings.client_credentials) {
    client_channel_cache_.try_emplace(
        std::string{client_name},
        testsuite_grpc.IsTlsEnabled() ? creds
                                      : grpc::InsecureChannelCredentials(),
        settings.channel_args, settings.channel_count);
  }
}

impl::ChannelCache::Token ClientFactory::GetChannel(
    const std::string& client_name, const std::string& endpoint) {
  // Spawn a blocking task creating a gRPC channel
  // This is third party code, no use of span inside it

  return engine::AsyncNoSpan(
             channel_task_processor_,
             [&] {
               if (auto* const channel_cache = utils::FindOrNullptr(
                       client_channel_cache_, client_name)) {
                 return channel_cache->Get(endpoint);
               }
               return channel_cache_.Get(endpoint);
             })
      .Get();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
