#include <userver/components/tcp_acceptor_base.hpp>

#include <userver/components/component.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <server/net/create_socket.hpp>
#include <server/net/listener_config.hpp>

#include <netinet/tcp.h>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

using server::net::ListenerConfig;

std::string SocketsTaskProcessorName(const ComponentConfig& config,
                                     const ListenerConfig& acceptor_config) {
  return config["sockets_task_processor"].As<std::string>(
      acceptor_config.task_processor);
}

}  // namespace

TcpAcceptorBase::TcpAcceptorBase(const ComponentConfig& config,
                                 const ComponentContext& context)
    : TcpAcceptorBase(config, context, config.As<ListenerConfig>()) {}

TcpAcceptorBase::~TcpAcceptorBase() = default;

yaml_config::Schema TcpAcceptorBase::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
# yaml
type: object
description: |
  Component for accepting incoming TCP connections and passing a
  socket to derived class
additionalProperties: false
properties:
  port:
      type: integer
      description: port to listen on
  unix-socket:
      type: string
      description: unix socket to listen on instead of listening on a port
      defaultDescription: ''
  task_processor:
      type: string
      description: task processor to accept incoming connections
  backlog:
      type: integer
      description: max count of new connections pending acceptance
      defaultDescription: 1024
  no_delay:
      type: boolean
      description: whether to set the TCP_NODELAY option on incoming sockets
      defaultDescription: true
  sockets_task_processor:
      type: string
      description: task processor to process accepted sockets
      defaultDescription: value of `task_processor`
)");
}

TcpAcceptorBase::TcpAcceptorBase(const ComponentConfig& config,
                                 const ComponentContext& context,
                                 const ListenerConfig& acceptor_config)
    : LoggableComponentBase(config, context),
      no_delay_(config["no_delay"].As<bool>(true)),
      acceptor_task_processor_(
          context.GetTaskProcessor(acceptor_config.task_processor)),
      sockets_task_processor_(context.GetTaskProcessor(
          SocketsTaskProcessorName(config, acceptor_config))),
      listen_sock_(server::net::CreateSocket(acceptor_config)) {}

void TcpAcceptorBase::KeepAccepting() {
  while (!engine::current_task::ShouldCancel()) {
    engine::io::Socket sock = listen_sock_.Accept({});

    tasks_.Detach(engine::AsyncNoSpan(
        sockets_task_processor_,
        [this](engine::io::Socket&& sock) {
          if (no_delay_) {
            sock.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);
          }
          ProcessSocket(std::move(sock));
        },
        std::move(sock)));
  }
}

void TcpAcceptorBase::OnAllComponentsLoaded() {
  // Start handling after the derived object was fully constructed

  // NOLINTNEXTLINE(cppcoreguidelines-slicing)
  acceptor_ = engine::AsyncNoSpan(acceptor_task_processor_,
                                  &TcpAcceptorBase::KeepAccepting, this);
}

void TcpAcceptorBase::OnAllComponentsAreStopping() {
  acceptor_ = {};  // Cancel and wait for finish
  listen_sock_.Close();
  tasks_.CancelAndWait();
}

}  // namespace components

USERVER_NAMESPACE_END
