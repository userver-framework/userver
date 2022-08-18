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

/// @ingroup userver_base_classes userver_components
///
/// @brief Component for accepting incomming TCP connections and passing a
/// socket to derived class.
class TcpAcceptorBase : public LoggableComponentBase {
 public:
  TcpAcceptorBase(const ComponentConfig&, const ComponentContext&);
  ~TcpAcceptorBase() override;

  static yaml_config::Schema GetStaticConfigSchema();

 protected:
  /// Override this function to process incomming sockets.
  ///
  /// @warning The function is called concurrently from multiple threads on
  /// each new socket.
  virtual void ProcessSocket(engine::io::Socket&& sock) = 0;

 private:
  TcpAcceptorBase(const ComponentConfig& config,
                  const ComponentContext& context,
                  const server::net::ListenerConfig& acceptor_config);

  void KeepAccepting();

  const bool no_delay_;
  engine::TaskProcessor& clients_task_processor_;
  const std::string name_;
  concurrent::BackgroundTaskStorage tasks_;
  engine::Task acceptor_;
  engine::io::Socket listen_sock_;
};

}  // namespace components

USERVER_NAMESPACE_END
