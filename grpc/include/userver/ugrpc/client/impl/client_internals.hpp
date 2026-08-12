#pragma once

#include <optional>
#include <string>

#include <grpcpp/support/channel_arguments.h>

#include <userver/dynamic_config/source.hpp>

#include <userver/concurrent/impl/striped_read_indicator.hpp>
#include <userver/ugrpc/client/client_settings.hpp>
#include <userver/ugrpc/client/fwd.hpp>
#include <userver/ugrpc/client/impl/channel_factory.hpp>
#include <userver/ugrpc/client/middlewares/fwd.hpp>
#include <userver/ugrpc/client/proxy_settings.hpp>
#include <userver/ugrpc/client/retry_config.hpp>
#include <userver/ugrpc/client/retry_limiter.hpp>
#include <userver/utils/statistics/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {
class StatisticsStorage;
class CompletionQueuePoolBase;
}  // namespace ugrpc::impl

namespace ugrpc::client::impl {

class ClientQosErrorsReporter;

/// Contains all non-code-generated dependencies for creating a gRPC client
struct ClientInternals final {
    // Must be first field to ensure it's destroyed last. Other fields may use
    // the ClientFactory, so this lock must outlive them. It keeps the
    // ClientFactory's alive_clients_indicator_ locked while this client exists,
    // allowing the factory to detect if any clients outlive it.
    concurrent::impl::StripedReadIndicatorLock alive_client_indicator_lock;

    std::string client_name;
    std::string destination_prefix_in_metrics;
    std::string endpoint;
    Middlewares middlewares;
    ugrpc::impl::CompletionQueuePoolBase& completion_queues;
    ugrpc::impl::StatisticsStorage& statistics_storage;
    ClientQosErrorsReporter& client_qos_error_reporter;
    dynamic_config::Source config_source;
    testsuite::GrpcControl& testsuite_grpc;
    const dynamic_config::Key<ClientQos>* qos{nullptr};
    std::size_t channel_count{};
    DedicatedMethodsConfig dedicated_methods_config;
    ChannelFactory channel_factory;
    const RetryConfig& retry_config;
    RetryLimiterFactory* retry_limiter_factory;
    const grpc::ChannelArguments& channel_args;
    const std::optional<std::string>& default_service_config;
    const ProxySettings& proxy_settings;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
