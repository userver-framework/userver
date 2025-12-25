#pragma once

/// @file userver/components/tcp_acceptor_base.hpp
/// @brief @copybrief components::TcpAcceptorBase

#include <userver/components/component_base.hpp>
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
/// @brief Component for accepting incoming TCP connections.
///
/// Each accepted socket is processed in a new coroutine by ProcessSocket of the derived class.
///
/// ## Static options of components::TcpAcceptorBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/tcp_acceptor_base.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// @see @ref scripts/docs/en/userver/tutorial/tcp_service.md
class TcpAcceptorBase : public ComponentBase {
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
    TcpAcceptorBase(
        const ComponentConfig& config,
        const ComponentContext& context,
        const server::net::ListenerConfig& acceptor_config
    );

    void KeepAccepting(engine::io::Socket& listen_sock);

    void Start();
    void Stop() noexcept;

    struct SocketData {
        engine::io::Socket listen_sock;
        engine::Task acceptor;
    };

    const bool no_delay_;
    engine::TaskProcessor& acceptor_task_processor_;
    engine::TaskProcessor& sockets_task_processor_;
    concurrent::BackgroundTaskStorageCore tasks_;
    std::vector<SocketData> sockets_;
};

}  // namespace components

USERVER_NAMESPACE_END
