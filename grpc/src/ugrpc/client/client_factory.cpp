#include <userver/ugrpc/client/client_factory.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

ClientFactory::ClientFactory(
    ClientFactorySettings&& client_factory_settings,
    engine::TaskProcessor& channel_task_processor,
    MiddlewareFactories mws,
    ugrpc::impl::CompletionQueuePoolBase& completion_queues,
    ugrpc::impl::StatisticsStorage& statistics_storage,
    testsuite::GrpcControl& testsuite_grpc,
    dynamic_config::Source config_source
)
    : client_factory_settings_(std::move(client_factory_settings)),
      channel_task_processor_(channel_task_processor),
      mws_(mws),
      completion_queues_(completion_queues),
      client_statistics_storage_(statistics_storage),
      config_source_(config_source),
      testsuite_grpc_(testsuite_grpc) {}

impl::ClientInternals ClientFactory::MakeClientInternals(ClientSettings&& client_settings) {
    UINVARIANT(!client_settings.client_name.empty(), "Client name is empty");
    UINVARIANT(!client_settings.endpoint.empty(), "Client endpoint is empty");

    auto mws = impl::InstantiateMiddlewares(mws_, client_settings.client_name);

    auto channel_credentials = testsuite_grpc_.IsTlsEnabled()
                                   ? GetClientCredentials(client_factory_settings_, client_settings.client_name)
                                   : grpc::InsecureChannelCredentials();

    impl::ChannelFactory channel_factory{
        channel_task_processor_, std::move(client_settings.endpoint), std::move(channel_credentials)};

    impl::ChannelArgumentsBuilder channel_arguments_builder{
        client_factory_settings_.channel_args, client_factory_settings_.default_service_config};

    return impl::ClientInternals{
        std::move(client_settings.client_name),
        std::move(mws),
        completion_queues_,
        client_statistics_storage_,
        config_source_,
        testsuite_grpc_,
        client_settings.client_qos,
        client_factory_settings_.channel_count,
        std::move(client_settings.dedicated_methods_config),
        std::move(channel_factory),
        std::move(channel_arguments_builder)};
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
