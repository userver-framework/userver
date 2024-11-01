#include <userver/ugrpc/client/client_factory.hpp>

#include <userver/engine/async.hpp>
#include <userver/utils/algo.hpp>

#include <ugrpc/impl/logging.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

ClientFactory::ClientFactory(
    ClientFactorySettings&& settings,
    engine::TaskProcessor& channel_task_processor,
    MiddlewareFactories mws,
    ugrpc::impl::CompletionQueuePoolBase& completion_queues,
    ugrpc::impl::StatisticsStorage& statistics_storage,
    testsuite::GrpcControl& testsuite_grpc,
    dynamic_config::Source source
)
    : settings_(std::move(settings)),
      channel_task_processor_(channel_task_processor),
      mws_(mws),
      completion_queues_(completion_queues),
      channel_cache_(
          testsuite_grpc.IsTlsEnabled() ? settings_.credentials : grpc::InsecureChannelCredentials(),
          settings_.channel_args,
          settings_.channel_count
      ),
      client_statistics_storage_(statistics_storage),
      config_source_(source),
      testsuite_grpc_(testsuite_grpc) {
    ugrpc::impl::SetupNativeLogging();
    ugrpc::impl::UpdateNativeLogLevel(settings_.native_log_level);

    for (auto& [client_name, creds] : settings_.client_credentials) {
        client_channel_cache_.try_emplace(
            std::string{client_name},
            testsuite_grpc.IsTlsEnabled() ? creds : grpc::InsecureChannelCredentials(),
            settings_.channel_args,
            settings_.channel_count
        );
    }
}

impl::ChannelCache::Token ClientFactory::GetChannel(const std::string& client_name, const std::string& endpoint) {
    // Spawn a blocking task creating a gRPC channel
    // This is third party code, no use of span inside it

    return engine::AsyncNoSpan(
               channel_task_processor_,
               [&] {
                   if (auto* const channel_cache = utils::FindOrNullptr(client_channel_cache_, client_name)) {
                       return channel_cache->Get(endpoint);
                   }
                   return channel_cache_.Get(endpoint);
               }
    ).Get();
}

impl::ClientDependencies ClientFactory::MakeClientDependencies(ClientSettings&& settings) {
    UINVARIANT(!settings.client_name.empty(), "Client name is empty");
    UINVARIANT(!settings.endpoint.empty(), "Client endpoint is empty");

    return impl::ClientDependencies{
        settings.client_name,
        settings.endpoint,
        impl::InstantiateMiddlewares(mws_, settings.client_name),
        completion_queues_,
        client_statistics_storage_,
        GetChannel(settings.client_name, settings.endpoint),
        config_source_,
        testsuite_grpc_,
        settings.client_qos,
        settings_,
        std::move(settings.dedicated_methods_config),
    };
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
