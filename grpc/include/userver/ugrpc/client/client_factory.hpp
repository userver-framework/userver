#pragma once

/// @file userver/ugrpc/client/client_factory.hpp
/// @brief @copybrief ugrpc::client::ClientFactory

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <grpcpp/completion_queue.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/support/channel_arguments.h>

#include <userver/dynamic_config/source.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/logging/level.hpp>
#include <userver/storages/secdist/secdist.hpp>
#include <userver/testsuite/grpc_control.hpp>

#include <userver/ugrpc/client/client_factory_settings.hpp>
#include <userver/ugrpc/client/fwd.hpp>
#include <userver/ugrpc/client/impl/channel_cache.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {
class StatisticsStorage;
class CompletionQueuePoolBase;
}  // namespace ugrpc::impl

namespace ugrpc::client {

/// Settings relating to creation of a code-generated client
struct ClientSettings final {
    /// **(Required)**
    /// The name of the client, for diagnostics, credentials and middlewares.
    std::string client_name;

    /// **(Required)**
    /// The URI to connect to, e.g. `http://my.domain.com:8080`.
    /// Should not include any HTTP path, just schema, domain name and port. Unix
    /// sockets are also supported. For details, see:
    /// https://grpc.github.io/grpc/cpp/md_doc_naming.html
    std::string endpoint;

    /// **(Optional)**
    /// The name of the QOS
    /// @ref scripts/docs/en/userver/dynamic_config.md "dynamic config"
    /// that will be applied automatically to every RPC.
    ///
    /// Timeout from QOS config is ignored if:
    ///
    /// * an explicit `qos` parameter is specified at RPC creation, or
    /// * deadline is specified in the `client_context` passed at RPC creation.
    ///
    /// ## Client QOS config definition sample
    ///
    /// @snippet grpc/tests/tests/unit_test_client_qos.hpp  qos config key
    const dynamic_config::Key<ClientQos>* client_qos{nullptr};

    /// **(Optional)**
    /// Dedicated high-load methods that have separate channels
    DedicatedMethodsConfig dedicated_methods_config{};
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
    template <typename Client>
    Client MakeClient(ClientSettings&& settings);

    /// @deprecated Use the overload taking @ref ClientSettings instead.
    /// @brief Make a client of the specified code-generated type.
    /// @param client_name see @ref ClientSettings
    /// @param endpoint see @ref ClientSettings
    template <typename Client>
    Client MakeClient(const std::string& client_name, const std::string& endpoint);

    /// @cond
    // For internal use only.
    ClientFactory(
        ClientFactorySettings&& settings,
        engine::TaskProcessor& channel_task_processor,
        MiddlewareFactories mws,
        ugrpc::impl::CompletionQueuePoolBase& queue,
        ugrpc::impl::StatisticsStorage& statistics_storage,
        testsuite::GrpcControl& testsuite_grpc,
        dynamic_config::Source source
    );
    /// @endcond

private:
    impl::ChannelCache::Token GetChannel(const std::string& client_name, const std::string& endpoint);

    impl::ClientDependencies MakeClientDependencies(ClientSettings&& settings);

    ClientFactorySettings settings_;
    engine::TaskProcessor& channel_task_processor_;
    MiddlewareFactories mws_;
    ugrpc::impl::CompletionQueuePoolBase& completion_queues_;
    impl::ChannelCache channel_cache_;
    std::unordered_map<std::string, impl::ChannelCache> client_channel_cache_;
    ugrpc::impl::StatisticsStorage& client_statistics_storage_;
    const dynamic_config::Source config_source_;
    testsuite::GrpcControl& testsuite_grpc_;
};

template <typename Client>
Client ClientFactory::MakeClient(ClientSettings&& settings) {
    return Client(MakeClientDependencies(std::move(settings)));
}

template <typename Client>
Client ClientFactory::MakeClient(const std::string& client_name, const std::string& endpoint) {
    ClientSettings settings;
    settings.client_name = client_name;
    settings.endpoint = endpoint;
    return MakeClient<Client>(std::move(settings));
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
