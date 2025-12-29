#pragma once

/// @file userver/server/websocket/websocket_handler.hpp
/// @brief @copybrief server::websocket::WebsocketHandlerBase

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/websocket/server.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::websocket {

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
    virtual void Handle(WebSocketConnection& websocket, server::request::RequestContext&) const = 0;

    /// @brief If any code is required for handshake validation,
    /// it goes here.
    virtual bool
    HandleHandshake(const server::http::HttpRequest&, server::http::HttpResponse&, server::request::RequestContext&)
        const {
        return true;
    }

    /// @cond
    void WriteMetrics(utils::statistics::Writer& writer) const;

    static yaml_config::Schema GetStaticConfigSchema();
    /// @endcond

    /// @brief If \a request isn't a websocket request the function handles a
    /// request.
    virtual void HandleNonWebsocketRequest(
        [[maybe_unused]] const server::http::HttpRequest& request,
        [[maybe_unused]] server::request::RequestContext& context
    ) const {
        LOG_WARNING() << "Not a GET 'Upgrade: websocket' and 'Connection: Upgrade' request";
        throw server::handlers::ClientError();
    }

private:
    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext& context)
        const override;

    websocket::Config config_;
    mutable Statistics stats_;
};

}  // namespace server::websocket

USERVER_NAMESPACE_END
