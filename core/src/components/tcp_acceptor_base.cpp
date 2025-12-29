#include <userver/components/tcp_acceptor_base.hpp>

#include <userver/components/component.hpp>
#include <userver/components/scope.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <server/net/create_socket.hpp>
#include <server/net/listener_config.hpp>

#include <netinet/tcp.h>

#ifndef ARCADIA_ROOT
#include "generated/src/components/tcp_acceptor_base.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

using server::net::ListenerConfig;

TcpAcceptorBase::TcpAcceptorBase(const ComponentConfig& config, const ComponentContext& context)
    : TcpAcceptorBase(config, context, config.As<ListenerConfig>())
{}

TcpAcceptorBase::~TcpAcceptorBase() = default;

yaml_config::Schema TcpAcceptorBase::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/components/tcp_acceptor_base.yaml");
}

TcpAcceptorBase::TcpAcceptorBase(
    const ComponentConfig& config,
    const ComponentContext& context,
    const ListenerConfig& acceptor_config
)
    : ComponentBase(config, context),
      no_delay_(config["no_delay"].As<bool>(true)),
      acceptor_task_processor_(
          acceptor_config.task_processor
              ? context.GetTaskProcessor(*acceptor_config.task_processor)
              : engine::current_task::GetTaskProcessor()
      ),
      sockets_task_processor_(
          config["sockets_task_processor"].IsMissing()
              ? acceptor_task_processor_
              : context.GetTaskProcessor(config["sockets_task_processor"].As<std::string>())
      )
{
    for (const auto& port : acceptor_config.ports) {
        auto socket = server::net::CreateSocket(acceptor_config, port);
        sockets_.emplace_back(SocketData{std::move(socket), {}});
    }

    context.RegisterScope(MakeScope([this] {
        Start();
        return utils::FastScopeGuard([this]() noexcept { Stop(); });
    }));
}

void TcpAcceptorBase::KeepAccepting(engine::io::Socket& listen_sock) {
    while (!engine::current_task::ShouldCancel()) {
        engine::io::Socket sock = listen_sock.Accept({});

        tasks_.Detach(engine::AsyncNoSpan(
            sockets_task_processor_,
            [this](engine::io::Socket&& sock) {
                if (no_delay_) {
                    sock.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);
                }
                ProcessSocket(std::move(sock));
            },
            std::move(sock)
        ));
    }
}

void TcpAcceptorBase::Start() {
    // Start handling after the derived object was fully constructed

    for (auto& socket_data : sockets_) {
        socket_data.acceptor =
            engine::AsyncNoSpan(
                acceptor_task_processor_,
                &TcpAcceptorBase::KeepAccepting,
                this,
                std::ref(socket_data.listen_sock)
            )
                .AsTask();
    }
}

void TcpAcceptorBase::Stop() noexcept {
    for (auto& socket_data : sockets_) {
        socket_data.acceptor.SyncCancel();
        socket_data.listen_sock.Close();
    }
    tasks_.CancelAndWait();
}

}  // namespace components

USERVER_NAMESPACE_END
