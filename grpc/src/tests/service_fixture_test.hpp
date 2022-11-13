#pragma once

#include <memory>

#include <grpcpp/channel.h>

#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/labels.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/testing.hpp>

#include <userver/ugrpc/client/client_factory.hpp>
#include <userver/ugrpc/server/server.hpp>
#include <userver/ugrpc/server/service_base.hpp>

USERVER_NAMESPACE_BEGIN

// Sets up a mini gRPC server using the provided service implementations
class GrpcServiceFixture : public ::testing::Test {
 protected:
  GrpcServiceFixture();
  ~GrpcServiceFixture() override;

  void RegisterService(ugrpc::server::ServiceBase& service);

  // Must be called after the services are registered
  void StartServer(ugrpc::client::ClientFactoryConfig&& config = {});

  // Must be called in the destructor of the derived fixture
  void StopServer() noexcept;

  template <typename Client>
  Client MakeClient() {
    return client_factory_->MakeClient<Client>(*endpoint_);
  }

  utils::statistics::Snapshot GetStatistics(
      std::string prefix,
      std::vector<utils::statistics::Label> require_labels = {});

  ugrpc::server::Server& GetServer() noexcept;

 private:
  utils::statistics::Storage statistics_storage_;
  ugrpc::server::Server server_;
  std::optional<std::string> endpoint_;
  std::optional<ugrpc::client::ClientFactory> client_factory_;
};

// Sets up a mini gRPC server using a single default-constructed service
// implementation
template <typename Service>
class GrpcServiceFixtureSimple : public GrpcServiceFixture {
 protected:
  GrpcServiceFixtureSimple() {
    RegisterService(service_);
    StartServer();
  }

  ~GrpcServiceFixtureSimple() override { StopServer(); }

  Service& GetService() { return service_; }

 private:
  Service service_{};
};

// Sets up a mini gRPC server using a single default-constructed service
// implementation. Will create the client with number of connections greater
// than one
template <typename Service>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class GrpcServiceFixtureMultichannel
    : public GrpcServiceFixture,
      public testing::WithParamInterface<std::size_t> {
 protected:
  GrpcServiceFixtureMultichannel() {
    RegisterService(service_);
    ugrpc::client::ClientFactoryConfig client_factory_config{};
    client_factory_config.channel_count = GetParam();
    StartServer(std::move(client_factory_config));
  }

  ~GrpcServiceFixtureMultichannel() override { StopServer(); }

 private:
  Service service_{};
};

USERVER_NAMESPACE_END
