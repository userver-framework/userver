#pragma once

/// @file userver/ugrpc/tests/standalone_client.hpp
/// @brief @copybrief ugrpc::tests::StandaloneClientFactory

#include <string>

#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/utils/statistics/storage.hpp>

#include <userver/testsuite/grpc_control.hpp>
#include <userver/ugrpc/client/client_factory.hpp>
#include <userver/ugrpc/client/impl/completion_queue_pool.hpp>
#include <userver/ugrpc/impl/statistics_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::tests {

/// @brief Sets up a mini gRPC client that is not directly associated with any
/// userver gRPC server.
///
/// @note Prefer ugrpc::tests::ServiceBase and friends by default to create
/// a userver gRPC service + client pair.
class StandaloneClientFactory final {
public:
    /// @param client_factory_settings Options settings for the internal
    /// ugrpc::client::ClientFactory.
    explicit StandaloneClientFactory(client::ClientFactorySettings&& client_factory_settings = {});

    /// @returns a client for the specified gRPC service, connected
    /// to the specified endpoint.
    /// @see GetFreeIpv6Port
    /// @see MakeIpv6Endpoint
    template <typename Client>
    Client MakeClient(const std::string& endpoint) {
        return client_factory_.MakeClient<Client>("test", endpoint);
    }

    /// @returns the internal ugrpc::client::ClientFactory.
    client::ClientFactory& GetClientFactory() { return client_factory_; }

private:
    utils::statistics::Storage statistics_storage_;
    ugrpc::impl::StatisticsStorage client_statistics_storage_{
        statistics_storage_,
        ugrpc::impl::StatisticsDomain::kClient};
    dynamic_config::StorageMock config_storage_{dynamic_config::MakeDefaultStorage({})};
    client::impl::CompletionQueuePool completion_queues_{1};
    testsuite::GrpcControl testsuite_control_{{}, false};
    client::ClientFactory client_factory_;
};

/// Acquire a free IPv6 port. Note: there is a small chance that a race could
/// occur, and the port will be taken immediately after this function returns.
/// Prefer spinning up a server with port auto-detection if possible.
std::uint16_t GetFreeIpv6Port();

/// Make an IPv6 localhost gRPC endpoint from port.
std::string MakeIpv6Endpoint(std::uint16_t port);

}  // namespace ugrpc::tests

USERVER_NAMESPACE_END
