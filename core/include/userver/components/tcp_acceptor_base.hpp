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
/// task_processor | task processor to accept incoming connections | the 'default_task_processor' value from components::ManagerControllerComponent
/// backlog | max count of new connections pending acceptance | 1024
/// no_delay | whether to set the `TCP_NODELAY` option on incoming sockets | true
/// sockets_task_processor | task processor to process accepted sockets | value of `task_processor`
///
/// @see @ref scripts/docs/en/userver/tutorial/tcp_service.md

// clang-format on
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

    void OnAllComponentsLoaded() final;
    void OnAllComponentsAreStopping() final;

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
