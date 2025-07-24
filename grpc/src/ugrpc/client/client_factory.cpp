#include <userver/ugrpc/client/client_factory.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

ClientFactory::ClientFactory(
    ClientFactorySettings&& client_factory_settings,
    engine::TaskProcessor& channel_task_processor,
    impl::MiddlewarePipelineCreator& middleware_pipeline_creator,
    ugrpc::impl::CompletionQueuePoolBase& completion_queues,
    ugrpc::impl::StatisticsStorage& statistics_storage,
    testsuite::GrpcControl& testsuite_grpc,
    dynamic_config::Source config_source
)
    : client_factory_settings_(std::move(client_factory_settings)),
      channel_task_processor_(channel_task_processor),
      middleware_pipeline_creator_(middleware_pipeline_creator),
      completion_queues_(completion_queues),
      client_statistics_storage_(statistics_storage),
      config_source_(config_source),
      testsuite_grpc_(testsuite_grpc) {}

impl::ClientInternals ClientFactory::MakeClientInternals(
    ClientSettings&& client_settings,
    std::optional<ugrpc::impl::StaticServiceMetadata> meta
) {
    UINVARIANT(!client_settings.client_name.empty(), "Client name is empty");
    UINVARIANT(!client_settings.endpoint.empty(), "Client endpoint is empty");

    ClientInfo info{
        /*client_name=*/client_settings.client_name,
        /*service_full_name=*/std::nullopt,
    };
    if (meta.has_value()) {
        info.service_full_name.emplace(meta.value().service_full_name);
    }

    auto middlewares = middleware_pipeline_creator_.CreateMiddlewares(info);

    auto channel_credentials = testsuite_grpc_.IsTlsEnabled()
                                   ? GetClientCredentials(client_factory_settings_, client_settings.client_name)
                                   : grpc::InsecureChannelCredentials();

    impl::ChannelFactory channel_factory{
        channel_task_processor_, std::move(client_settings.endpoint), std::move(channel_credentials)};

    return impl::ClientInternals{
        std::move(client_settings.client_name),
        std::move(middlewares),
        completion_queues_,
        client_statistics_storage_,
        config_source_,
        testsuite_grpc_,
        client_settings.client_qos,
        client_factory_settings_.channel_count,
        std::move(client_settings.dedicated_methods_config),
        std::move(channel_factory),
        client_factory_settings_.channel_args,
        client_factory_settings_.default_service_config,
    };
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
