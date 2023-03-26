#pragma once

/// @file userver/components/tcp_acceptor_base.hpp
/// @brief @copybrief components::TcpAcceptorBase

#include <userver/components/loggable_component_base.hpp>
#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/task/task.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {
struct ListenerConfig;
}

namespace components {

// clang-format off

/// @ingroup userver_base_classes userver_components
///
/// @brief Component for accepting incoming TCP connections.
///
/// Each accepted socket is processed in a new coroutine by ProcessSocket of
/// the derived class.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// port | port to listen on | -
/// unix-socket | unix socket to listen on instead of listening on a port | ''
/// task_processor | task processor to accept incoming connections | -
/// backlog | max count of new connections pending acceptance | 1024
/// no_delay | whether to set the `TCP_NODELAY` option on incoming sockets | true
/// sockets_task_processor | task processor to process accepted sockets | value of `task_processor`
///
/// @see @ref md_en_userver_tutorial_tcp_service

// clang-format on
class TcpAcceptorBase : public LoggableComponentBase {
 public:
  TcpAcceptorBase(const ComponentConfig&, const ComponentContext&);
  ~TcpAcceptorBase() override;

  static yaml_config::Schema GetStaticConfigSchema();

 protected:
  /// Override this function to process incoming sockets.
  ///
  /// @warning The function is called concurrently from multiple threads on
  /// each new socket.
  virtual void ProcessSocket(engine::io::Socket&& sock) = 0;

 private:
  TcpAcceptorBase(const ComponentConfig& config,
                  const ComponentContext& context,
                  const server::net::ListenerConfig& acceptor_config);

  void KeepAccepting();

  void OnAllComponentsLoaded() final;
  void OnAllComponentsAreStopping() final;

  const bool no_delay_;
  engine::TaskProcessor& acceptor_task_processor_;
  engine::TaskProcessor& sockets_task_processor_;
  concurrent::BackgroundTaskStorageCore tasks_;
  engine::io::Socket listen_sock_;
  engine::Task acceptor_;
};

}  // namespace components

USERVER_NAMESPACE_END
