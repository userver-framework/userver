#pragma once

/// @file userver/ugrpc/server/server.hpp
/// @brief @copybrief ugrpc::server::Server

#include <memory>

#include <grpcpp/completion_queue.h>
#include <grpcpp/server_builder.h>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>

#include <userver/ugrpc/server/reactor.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @brief Manages the gRPC server
///
/// All methods are thread-safe unless specified otherwise.
/// Usually retrieved from ugrpc::server::ServerComponent.
class Server final {
 public:
  /// @brief Start building the server
  Server();

  Server(Server&&) = delete;
  Server& operator=(Server&&) = delete;
  ~Server();

  /// @brief Add a port to listen to
  /// @note Only 1 port may be registered using this interface. Use
  /// `GetServerBuilder` for advanced configuration.
  /// @note Specify `0` to pick a free port automatically.
  void AddListeningPort(int port);

  /// @brief Register a handler in the server
  template <typename Handler>
  void AddHandler(std::unique_ptr<Handler>&& handler,
                  engine::TaskProcessor& task_processor) {
    AddReactor(Handler::MakeReactor(
        std::move(handler), GetServerCompletionQueue(), task_processor));
  }

  /// @brief For advanced configuration of the gRPC server
  /// @note Customizing the ServerBuilder directly is not thread-safe with
  /// respect to Server's non-const methods.
  ::grpc::ServerBuilder& GetServerBuilder() noexcept;

  /// @returns the completion queue for clients
  /// @note All RPCs are cancelled on 'Stop'. If you need to perform requests
  /// after the server has been closed, create an ugrpc::client::QueueHolder -
  /// usually no more than one instance per program.
  ::grpc::CompletionQueue& GetCompletionQueue() noexcept;

  /// @brief Start accepting requests
  /// @note Must be called at most once after all the handlers are registered
  void Start();

  /// @returns The port assigned using `AddListeningPort`
  /// @note Only available after 'Start' has returned
  int GetPort() const noexcept;

  /// @brief Stop accepting requests
  /// @note Should be called once before the handlers are destroyed
  void Stop() noexcept;

 private:
  ::grpc::ServerCompletionQueue& GetServerCompletionQueue() noexcept;

  void AddReactor(std::unique_ptr<Reactor>&& reactor);

  class Impl;
  utils::FastPimpl<Impl, 592, 8> impl_;
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
