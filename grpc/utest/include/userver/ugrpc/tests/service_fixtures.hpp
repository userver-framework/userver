#pragma once

#include <userver/ugrpc/tests/service.hpp>

#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/utils/statistics/testing.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::tests {

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServiceFixtureBase : protected ServiceBase, public ::testing::Test {};

template <typename GrpcService>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServiceFixture : protected Service<GrpcService>, public ::testing::Test {
 protected:
  using Service<GrpcService>::Service;

  utils::statistics::Snapshot GetStatistics(
      std::string prefix,
      std::vector<utils::statistics::Label> require_labels = {}) {
    return utils::statistics::Snapshot{this->GetStatisticsStorage(),
                                       std::move(prefix),
                                       std::move(require_labels)};
  }
};

// Sets up a mini gRPC server using a single default-constructed service
// implementation. Will create the client with number of connections greater
// than one
template <typename GrpcService>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServiceFixtureMultichannel
    : public ::testing::Test,
      public testing::WithParamInterface<std::size_t>,
      protected ServiceMultichannel<GrpcService> {
 protected:
  ServiceFixtureMultichannel() : ServiceMultichannel<GrpcService>(GetParam()) {}

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
