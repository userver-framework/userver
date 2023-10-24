#pragma once

#include <memory>
#include <utility>

#include <grpcpp/channel.h>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/utils/statistics/labels.hpp>
#include <userver/utils/statistics/storage.hpp>

#include <userver/ugrpc/client/client_factory.hpp>
#include <userver/ugrpc/server/server.hpp>
#include <userver/ugrpc/server/service_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::tests {

/// Sets up a mini gRPC server using the provided service implementations
class ServiceBase {
 public:
  explicit ServiceBase(dynamic_config::StorageMock&& dynconf,
                       server::ServerConfig&& server_config);

  ServiceBase() : ServiceBase(dynamic_config::MakeDefaultStorage({}), {}) {}

  virtual ~ServiceBase();

  void RegisterService(server::ServiceBase& service);

  /// Must be called after the services are registered
  void StartServer(client::ClientFactoryConfig&& config = {});

  void StopServer() noexcept;

  template <typename Client>
  Client MakeClient() {
    return client_factory_->MakeClient<Client>("test", *endpoint_);
  }

  dynamic_config::Source GetConfigSource() const;

  server::Server& GetServer() noexcept;

  /// Server middlewares can be modified before the first RegisterService call
  void AddServerMiddleware(std::shared_ptr<server::MiddlewareBase> middleware);

  /// Client middlewares can be modified before the first RegisterService call
  void AddClientMiddleware(
      std::shared_ptr<const client::MiddlewareFactoryBase> middleware_factory);

  void ExtendDynamicConfig(const std::vector<dynamic_config::KeyValue>&);

  utils::statistics::Storage& GetStatisticsStorage() {
    return statistics_storage_;
  }

 private:
  utils::statistics::Storage statistics_storage_;
  dynamic_config::StorageMock config_storage_;
  server::Server server_;
  server::Middlewares server_middlewares_;
  client::MiddlewareFactories middleware_factories_;
  bool adding_middlewares_allowed_{true};
  testsuite::GrpcControl testsuite_;
  std::optional<std::string> endpoint_;
  std::optional<client::ClientFactory> client_factory_;
};

/// Sets up a mini gRPC server using a single default-constructed service
/// implementation
template <typename GrpcService>
class Service : public ServiceBase {
 public:
  template <typename... Args>
  Service(Args&&... args) : ServiceBase(std::forward<Args>(args)...) {
    RegisterService(service_);
    StartServer();
  }

  ~Service() override { StopServer(); }

  GrpcService& GetService() { return service_; }

 private:
  GrpcService service_{};
};

/// Sets up a mini gRPC server using a single default-constructed service
/// implementation. Will create the client with number of connections greater
/// than one
template <typename GrpcService>
class ServiceMultichannel : public ServiceBase {
 protected:
  ServiceMultichannel(std::size_t channel_count) {
    UASSERT(channel_count >= 1);
    RegisterService(service_);
    client::ClientFactoryConfig client_factory_config{};
    client_factory_config.channel_count = channel_count;
    StartServer(std::move(client_factory_config));
  }

  ~ServiceMultichannel() override { StopServer(); }

  GrpcService& GetService() { return service_; }

 private:
  GrpcService service_{};
};

}  // namespace ugrpc::tests

USERVER_NAMESPACE_END
