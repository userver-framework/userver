#pragma once

/// @file userver/ugrpc/server/server.hpp
/// @brief @copybrief ugrpc::server::Server

#include <functional>

#include <grpcpp/completion_queue.h>
#include <grpcpp/server_builder.h>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/yaml_config/fwd.hpp>

#include <userver/logging/level.hpp>
#include <userver/ugrpc/server/service_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// Settings relating to the whole gRPC server
struct ServerConfig final {
  /// The port to listen to. If `0`, a free port will be picked automatically.
  /// If none, the ports have to be configured programmatically using
  /// Server::WithServerBuilder.
  std::optional<int> port{};
};

ServerConfig Parse(const yaml_config::YamlConfig& value,
                   formats::parse::To<ServerConfig> to);

/// @brief Manages the gRPC server
///
/// All methods are thread-safe.
/// Usually retrieved from ugrpc::server::ServerComponent.
class Server final {
 public:
  using SetupHook = std::function<void(grpc::ServerBuilder&)>;

  /// @brief Start building the server
  explicit Server(ServerConfig&& config);

  Server(Server&&) = delete;
  Server& operator=(Server&&) = delete;
  ~Server();

  /// @brief Register a service implementation in the server. The user or the
  /// component is responsible for keeping `service` alive at least until `Stop`
  /// is called.
  void AddService(ServiceBase& service, engine::TaskProcessor& task_processor);

  /// @brief For advanced configuration of the gRPC server
  /// @note The ServerBuilder must not be stored and used outside of `setup`.
  void WithServerBuilder(SetupHook&& setup);

  /// @returns the completion queue for clients
  /// @note All RPCs are cancelled on 'Stop'. If you need to perform requests
  /// after the server has been closed, create an ugrpc::client::QueueHolder -
  /// usually no more than one instance per program.
  grpc::CompletionQueue& GetCompletionQueue() noexcept;

  /// @brief Start accepting requests
  /// @note Must be called at most once after all the services are registered
  void Start();

  /// @returns The port assigned using `AddListeningPort`
  /// @note Only available after 'Start' has returned
  int GetPort() const noexcept;

  /// @brief Stop accepting requests
  /// @note Should be called once before the services are destroyed
  void Stop() noexcept;

 private:
  class Impl;
  utils::FastPimpl<Impl, 592, 8> impl_;
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
