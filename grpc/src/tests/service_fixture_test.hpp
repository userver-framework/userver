#pragma once

#include <memory>

#include <grpcpp/channel.h>

#include <userver/formats/json/value.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/storage.hpp>

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
  void StartServer();

  // Must be called in the destructor of the derived fixture
  void StopServer() noexcept;

  template <typename Client>
  Client MakeClient() {
    return client_factory_->MakeClient<Client>(*endpoint_);
  }

  formats::json::Value GetStatistics();

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

  ~GrpcServiceFixtureSimple() { StopServer(); }

 private:
  Service service_{};
};

USERVER_NAMESPACE_END
