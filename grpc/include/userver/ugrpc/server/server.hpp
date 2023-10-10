#pragma once

/// @file userver/ugrpc/server/server.hpp
/// @brief @copybrief ugrpc::server::Server

#include <functional>
#include <memory>
#include <unordered_map>

#include <grpcpp/completion_queue.h>
#include <grpcpp/server_builder.h>

#include <userver/dynamic_config/source.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/logging/level.hpp>
#include <userver/logging/null_logger.hpp>
#include <userver/utils/function_ref.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/yaml_config/fwd.hpp>

#include <userver/ugrpc/impl/statistics.hpp>
#include <userver/ugrpc/server/middlewares/fwd.hpp>
#include <userver/ugrpc/server/service_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// Settings relating to the whole gRPC server
struct ServerConfig final {
  /// The port to listen to. If `0`, a free port will be picked automatically.
  /// If none, the ports have to be configured programmatically using
  /// Server::WithServerBuilder.
  std::optional<int> port{0};

  /// Number of completion queues to create. Should be ~2 times less than number
  /// of worker threads for best RPS.
  int completion_queue_num{2};

  /// Optional grpc-core channel args
  /// @see https://grpc.github.io/grpc/core/group__grpc__arg__keys.html
  std::unordered_map<std::string, std::string> channel_args{};

  /// The logging level override for the internal grpcpp library. Must be either
  /// `kDebug`, `kInfo` or `kError`.
  logging::Level native_log_level{logging::Level::kError};

  /// Serve a web page with runtime info about gRPC connections
  bool enable_channelz{false};

  /// 'access-tskv.log' logger
  logging::LoggerPtr access_tskv_logger{logging::MakeNullLogger()};
};

/// @brief Manages the gRPC server
///
/// All methods are thread-safe.
/// Usually retrieved from ugrpc::server::ServerComponent.
class Server final {
 public:
  using SetupHook = utils::function_ref<void(grpc::ServerBuilder&)>;

  /// @brief Start building the server
  explicit Server(ServerConfig&& config,
                  utils::statistics::Storage& statistics_storage,
                  dynamic_config::Source config_source);

  Server(Server&&) = delete;
  Server& operator=(Server&&) = delete;
  ~Server();

  /// @brief Register a service implementation in the server. The user or the
  /// component is responsible for keeping `service` and `middlewares` alive at
  /// least until `Stop` is called.
  void AddService(ServiceBase& service, ServiceConfig&& config);

  /// @brief Get names of all registered services
  std::vector<std::string_view> GetServiceNames() const;

  /// @brief For advanced configuration of the gRPC server
  /// @note The ServerBuilder must not be stored and used outside of `setup`.
  void WithServerBuilder(SetupHook setup);

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

  /// @brief Stop accepting requests. Also destroys server statistics and closes
  /// the associated CompletionQueue.
  /// @note Should be called at least once before the services are destroyed
  void Stop() noexcept;

  /// @cond
  /// Same as Stop, but:
  /// - does not destroy server statistics
  /// - does not close the associated CompletionQueue
  /// Stop must still be called. StopDebug is useful for testing.
  void StopDebug() noexcept;
  /// @endcond

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
