#pragma once

/// @file userver/ugrpc/client/client_factory.hpp
/// @brief @copybrief ugrpc::client::ClientFactory

#include <optional>
#include <string>
#include <utility>

#include <userver/dynamic_config/source.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/ugrpc/client/client_factory_settings.hpp>
#include <userver/ugrpc/client/client_settings.hpp>
#include <userver/ugrpc/client/fwd.hpp>
#include <userver/ugrpc/client/impl/client_internals.hpp>
#include <userver/ugrpc/client/impl/client_qos_errors_reporter.hpp>
#include <userver/ugrpc/client/middlewares/pipeline.hpp>
#include <userver/ugrpc/impl/static_service_metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {
class StatisticsStorage;
class CompletionQueuePoolBase;
}  // namespace ugrpc::impl

namespace ugrpc::client {

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
    template <typename Client>
    Client MakeClient(ClientSettings&& client_settings);

    /// @deprecated Use the overload taking @ref ClientSettings instead.
    /// @brief Make a client of the specified code-generated type.
    /// @param client_name see @ref ClientSettings
    /// @param endpoint see @ref ClientSettings
    template <typename Client>
    Client MakeClient(const std::string& client_name, const std::string& endpoint);

    /// @cond
    // For internal use only.
    ClientFactory(
        ClientFactorySettings&& client_factory_settings,
        engine::TaskProcessor& channel_task_processor,
        impl::MiddlewarePipelineCreator& middleware_pipeline_creator,
        ugrpc::impl::CompletionQueuePoolBase& completion_queues,
        ugrpc::impl::StatisticsStorage& statistics_storage,
        utils::statistics::MetricsStorage& metrics_storage,
        testsuite::GrpcControl& testsuite_grpc,
        dynamic_config::Source config_source
    );
    /// @endcond

private:
    impl::ClientInternals MakeClientInternals(
        ClientSettings&& client_settings,
        std::optional<ugrpc::impl::StaticServiceMetadata> meta
    );

    ClientFactorySettings client_factory_settings_;
    engine::TaskProcessor& channel_task_processor_;
    impl::MiddlewarePipelineCreator& middleware_pipeline_creator_;
    ugrpc::impl::CompletionQueuePoolBase& completion_queues_;
    ugrpc::impl::StatisticsStorage& client_statistics_storage_;
    impl::ClientQosErrorsReporter client_qos_errors_reporter_;
    const dynamic_config::Source config_source_;
    testsuite::GrpcControl& testsuite_grpc_;
};

template <typename Client>
Client ClientFactory::MakeClient(ClientSettings&& client_settings) {
    return Client(MakeClientInternals(std::move(client_settings), Client::GetMetadata()));
}

template <typename Client>
Client ClientFactory::MakeClient(const std::string& client_name, const std::string& endpoint) {
    ClientSettings client_settings;
    client_settings.client_name = client_name;
    client_settings.endpoint = endpoint;
    return MakeClient<Client>(std::move(client_settings));
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
