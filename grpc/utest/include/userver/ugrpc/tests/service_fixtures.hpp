#pragma once

/// @file userver/ugrpc/tests/service_fixtures.hpp
/// @brief gtest fixtures for testing ugrpc service implementations.  Requires
/// linking to `userver::grpc-utest`.

#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/labels.hpp>
#include <userver/utils/statistics/testing.hpp>

#include <userver/ugrpc/tests/service.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::tests {

/// @see @ref ugrpc::tests::ServiceBase
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServiceFixtureBase : protected ServiceBase, public ::testing::Test {
protected:
    /// @returns the statistics of the server and clients.
    utils::statistics::Snapshot GetStatistics(
        std::string prefix,
        std::vector<utils::statistics::Label> require_labels = {}
    );
};

/// @see @ref ugrpc::tests::Service
template <typename GrpcService>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServiceFixture : protected Service<GrpcService>, public ::testing::Test {
protected:
    using Service<GrpcService>::Service;

    /// @returns the statistics of the server and clients.
    utils::statistics::Snapshot GetStatistics(
        std::string prefix,
        std::vector<utils::statistics::Label> require_labels = {}
    ) {
        return utils::statistics::Snapshot{this->GetStatisticsStorage(), std::move(prefix), std::move(require_labels)};
    }
};

/// @brief Sets up a mini gRPC server and construct a client for it.
template <typename GrpcService, typename ClientType>
class ServiceWithClientFixture : public ServiceFixture<GrpcService> {
public:
    /// Passes @a args to the service fixture.
    template <typename... Args>
    explicit ServiceWithClientFixture(Args&&... args)
        : ServiceFixture<GrpcService>(std::forward<Args>(args)...),
          client_(this->ServiceFixture<GrpcService>::template MakeClient<ClientType>())
    {}

    ClientType& GetClient() { return client_; }

private:
    ClientType client_;
};

}  // namespace ugrpc::tests

USERVER_NAMESPACE_END
