#pragma once

#include <optional>
#include <string>

#include <grpcpp/support/channel_arguments.h>

#include <userver/dynamic_config/source.hpp>
#include <userver/testsuite/grpc_control.hpp>

#include <userver/ugrpc/client/client_settings.hpp>
#include <userver/ugrpc/client/impl/channel_factory.hpp>
#include <userver/ugrpc/client/middlewares/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {
class StatisticsStorage;
class CompletionQueuePoolBase;
}  // namespace ugrpc::impl

namespace ugrpc::client::impl {

/// Contains all non-code-generated dependencies for creating a gRPC client
struct ClientInternals final {
    std::string client_name;
    Middlewares middlewares;
    ugrpc::impl::CompletionQueuePoolBase& completion_queues;
    ugrpc::impl::StatisticsStorage& statistics_storage;
    dynamic_config::Source config_source;
    testsuite::GrpcControl& testsuite_grpc;
    const dynamic_config::Key<ClientQos>* qos{nullptr};
    std::size_t channel_count{};
    DedicatedMethodsConfig dedicated_methods_config;
    ChannelFactory channel_factory;
    const grpc::ChannelArguments& channel_args{};
    const std::optional<std::string>& default_service_config;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
