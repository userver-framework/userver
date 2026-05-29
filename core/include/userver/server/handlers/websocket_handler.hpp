#pragma once

/// @file userver/server/handlers/websocket_handler.hpp
/// @brief @copybrief server::handlers::WebsocketHandlerBase

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/websocket/connection.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

/// @ingroup userver_components userver_http_handlers userver_base_classes
///
/// @brief Base class for WebSocket handler
///
/// ## Static options of server::websocket::WebsocketHandlerBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/server/websocket/websocket_handler.md
///
/// Options inherited from @ref server::handlers::HttpHandlerBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/server/handlers/http_handler_base.md
///
/// Options inherited from @ref server::handlers::HandlerBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/server/handlers/handler_base.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// ## Example usage:
///
/// @snippet samples/websocket_service/main.cpp Websocket service sample - component
class WebsocketHandlerBase : public server::handlers::HttpHandlerBase {
public:
    WebsocketHandlerBase(const components::ComponentConfig&, const components::ComponentContext&);

    /// @brief Websocket handler code belongs here.
    virtual void Handle(websocket::WebSocketConnection& websocket, server::request::RequestContext&) const = 0;

    /// @brief If any code is required for handshake validation,
    /// it goes here.
    /// @return \a true to accept the WebSocket connection, \a false to reject
    /// it (handshake will be aborted and connection will not be established).
    virtual bool HandleHandshake(server::http::HttpRequest&, server::request::RequestContext&) const { return true; }

    /// @cond
    void WriteMetrics(utils::statistics::Writer& writer) const;

    static yaml_config::Schema GetStaticConfigSchema();
    /// @endcond

    bool IsWebsocketRequest(const server::http::HttpRequest& request) const;

    /// @brief If \a request isn't a websocket request the function handles a
    /// request.
    virtual std::string HandleNonWebsocketRequest(
        server::http::HttpRequest& request,
        server::request::RequestContext& context
    ) const;

    /// @brief If \a request is a websocket request, performs handshake and upgrade.
    virtual void HandleWebsocketRequest(server::http::HttpRequest& request, server::request::RequestContext& context)
        const;

protected:
    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext& context)
        const override;

private:
    websocket::Config config_;
    mutable websocket::Statistics stats_;
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
