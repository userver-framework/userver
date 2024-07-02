#pragma once

/// @file userver/ugrpc/tests/service_fixtures.hpp
/// @brief gtest fixtures for testing ugrpc service implementations.  Requires
/// linking to `userver::grpc-utest` and `userver::utest`.

#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/labels.hpp>
#include <userver/utils/statistics/testing.hpp>

#include <userver/ugrpc/tests/service.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::tests {

/// @see @ref ugrpc::tests::ServiceBase
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServiceFixtureBase : protected ServiceBase, public ::testing::Test {};

/// @see @ref ugrpc::tests::Service
template <typename GrpcService>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServiceFixture : protected Service<GrpcService>, public ::testing::Test {
 protected:
  using Service<GrpcService>::Service;

  /// @returns the statistics of the server and clients.
  utils::statistics::Snapshot GetStatistics(
      std::string prefix,
      std::vector<utils::statistics::Label> require_labels = {}) {
    return utils::statistics::Snapshot{this->GetStatisticsStorage(),
                                       std::move(prefix),
                                       std::move(require_labels)};
  }
};

}  // namespace ugrpc::tests

USERVER_NAMESPACE_END
