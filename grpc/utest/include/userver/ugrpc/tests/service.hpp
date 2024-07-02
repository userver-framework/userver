#pragma once

/// @file userver/ugrpc/tests/service.hpp
/// @brief Base classes for testing and benchmarking ugrpc service
/// implementations. Requires linking to `userver::grpc-utest`.

#include <memory>
#include <utility>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/utils/statistics/storage.hpp>

#include <userver/ugrpc/client/client_factory.hpp>
#include <userver/ugrpc/server/server.hpp>
#include <userver/ugrpc/server/service_base.hpp>

USERVER_NAMESPACE_BEGIN

/// userver gRPC testing facilities
namespace ugrpc::tests {

/// Sets up a mini gRPC server using the provided service implementations.
class ServiceBase {
 public:
  ServiceBase();

  explicit ServiceBase(server::ServerConfig&& server_config);

  ServiceBase(ServiceBase&&) = delete;
  ServiceBase& operator=(ServiceBase&&) = delete;
  virtual ~ServiceBase();

  /// Register a gRPC service implementation. The caller owns the service and
  /// should ensure that the services live at least until StopServer is called.
  void RegisterService(server::ServiceBase& service);

  /// Starts the server and connects a grpc channel to it.
  /// Should be called after the services are registered.
  void StartServer(client::ClientFactorySettings&& settings = {});

  /// Should be called before the registered services are destroyed.
  /// Should typically be called in the destructor of your gtest fixture.
  void StopServer() noexcept;

  /// @returns a client for the specified gRPC service, connected to the server.
  template <typename Client>
  Client MakeClient() {
    return client_factory_->MakeClient<Client>("test", *endpoint_);
  }

  /// @returns the stored @ref server::Server for advanced tweaking.
  server::Server& GetServer() noexcept;

  /// Server middlewares can be modified before the first RegisterService call.
  void AddServerMiddleware(std::shared_ptr<server::MiddlewareBase> middleware);

  /// Client middlewares can be modified before the first RegisterService call.
  void AddClientMiddleware(
      std::shared_ptr<const client::MiddlewareFactoryBase> middleware_factory);

  /// Modifies the internal dynamic configs storage. It is used by the server
  /// and clients, and is accessible through @ref GetConfigSource.
  /// Initially, the configs are filled with compile-time defaults.
  void ExtendDynamicConfig(const std::vector<dynamic_config::KeyValue>&);

  /// @returns the dynamic configs affected by @ref ExtendDynamicConfig.
  dynamic_config::Source GetConfigSource() const;

  /// @returns the statistics storage used by the server and clients.
  utils::statistics::Storage& GetStatisticsStorage();

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

/// @brief Sets up a mini gRPC server using a single service implementation.
/// @see @ref ugrpc::tests::ServiceBase
template <typename GrpcService>
class Service : public ServiceBase {
 public:
  /// Default-constructs the service.
  Service() : Service(std::in_place) {}

  /// Passes @a args to the service.
  template <typename... Args>
  explicit Service(std::in_place_t, Args&&... args)
      : Service(server::ServerConfig{}, std::in_place,
                std::forward<Args>(args)...) {}

  /// Passes @a args to the service.
  template <typename... Args>
  Service(server::ServerConfig&& server_config, std::in_place_t = std::in_place,
          Args&&... args)
      : ServiceBase(std::move(server_config)),
        service_(std::forward<Args>(args)...) {
    RegisterService(service_);
    StartServer();
  }

  ~Service() override { StopServer(); }

  /// @returns the stored service.
  GrpcService& GetService() { return service_; }

 private:
  GrpcService service_{};
};

}  // namespace ugrpc::tests

USERVER_NAMESPACE_END
