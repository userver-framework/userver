#include <userver/ugrpc/client/client_factory.hpp>

#include <ugrpc/impl/grpc_native_logging.hpp>

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
      client_statistics_storage_(statistics_storage),
      config_source_(source),
      testsuite_grpc_(testsuite_grpc) {
    ugrpc::impl::SetupNativeLogging();
    ugrpc::impl::UpdateNativeLogLevel(settings_.native_log_level);
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
        config_source_,
        testsuite_grpc_,
        settings.client_qos,
        settings_,
        channel_task_processor_,
        std::move(settings.dedicated_methods_config),
    };
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
