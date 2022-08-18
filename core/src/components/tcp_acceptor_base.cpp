#include <userver/components/tcp_acceptor_base.hpp>

#include <userver/components/component.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/schema.hpp>

#include <server/net/create_socket.hpp>
#include <server/net/listener_config.hpp>

#include <netinet/tcp.h>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

using server::net::ListenerConfig;

std::string ClientsTaskProcessorName(const ComponentConfig& config,
                                     const ListenerConfig& acceptor_config) {
  return config["clients_task_processor"].As<std::string>(
      acceptor_config.task_processor);
}

}  // namespace

TcpAcceptorBase::TcpAcceptorBase(const ComponentConfig& config,
                                 const ComponentContext& context)
    : TcpAcceptorBase(config, context, config.As<ListenerConfig>()) {}

TcpAcceptorBase::~TcpAcceptorBase() = default;

yaml_config::Schema TcpAcceptorBase::GetStaticConfigSchema() { return {}; }

TcpAcceptorBase::TcpAcceptorBase(const ComponentConfig& config,
                                 const ComponentContext& context,
                                 const ListenerConfig& acceptor_config)
    : LoggableComponentBase(config, context),
      no_delay_(config["no_delay"].As<bool>(true)),
      name_(config.Name()),
      clients_task_processor_(context.GetTaskProcessor(
          ClientsTaskProcessorName(config, acceptor_config))),
      listen_sock_(server::net::CreateSocket(acceptor_config)) {
  acceptor_ = engine::AsyncNoSpan(
      context.GetTaskProcessor(acceptor_config.task_processor),
      &TcpAcceptorBase::KeepAccepting, this);
}

void TcpAcceptorBase::KeepAccepting() {
  std::size_t sock_number = 0;
  while (!engine::current_task::ShouldCancel()) {
    engine::io::Socket sock = listen_sock_.Accept({});
    ++sock_number;

    tasks_.AsyncDetach(
        clients_task_processor_, fmt::format("{}_sock_{}", name_, sock_number),
        &TcpAcceptorBase::HandleNewSocket, this, std::move(sock));
  }
}

void TcpAcceptorBase::HandleNewSocket(engine::io::Socket&& sock) {
  try {
    if (no_delay_) {
      sock.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);
    }

    ProcessSocket(std::move(sock));
  } catch (const engine::io::IoCancelled& e) {
    return;
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Can't accept connection: " << ex;
  }
}

}  // namespace components

USERVER_NAMESPACE_END
