#pragma once

#include <userver/ugrpc/tests/service.hpp>
#include <userver/utils/statistics/testing.hpp>

USERVER_NAMESPACE_BEGIN

namespace tests {

/// Sets up a mini gRPC server using a single default-constructed service
/// implementation. Will create the client with number of connections
/// dependening on the UTEST_P parameter value.
template <typename GrpcService>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class ServiceFixtureMultichannel
    : public ::testing::Test,
      public testing::WithParamInterface<std::size_t>,
      protected ugrpc::tests::ServiceBase {
 protected:
  ServiceFixtureMultichannel() {
    const auto channel_count = GetParam();
    UASSERT(channel_count >= 1);
    RegisterService(service_);
    ugrpc::client::ClientFactorySettings client_factory_settings{};
    client_factory_settings.channel_count = channel_count;
    StartServer(std::move(client_factory_settings));
  }

  ~ServiceFixtureMultichannel() override { StopServer(); }

  GrpcService& GetService() { return service_; }

 private:
  GrpcService service_{};
};

}  // namespace tests

USERVER_NAMESPACE_END
