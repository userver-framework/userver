#pragma once

/// @file userver/server/websocket/websocket_handler.hpp
/// @brief @copybrief websocket::WebsocketHandlerBase

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/websocket/server.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::websocket {

// clang-format off

/// @ingroup userver_components userver_http_handlers userver_base_classes
///
/// @brief Base class for WebSocket handler
///
/// ## Static options:
/// Inherits all the options from server::handlers::HandlerBase and adds the
/// following ones:
///
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// log-level | overrides log level for this handle | <no override>
/// status-codes-log-level | map of "status": log_level items to override span log level for specific status codes | {}
/// max-remote-payload | max remote payload size | 65536
/// fragment-size | max output fragment size | 65536
///
/// ## Example usage:
///
/// @snippet samples/websocket_service/websocket_service.cpp Websocket service sample - component

// clang-format on
class WebsocketHandlerBase : public server::handlers::HttpHandlerBase {
 public:
  WebsocketHandlerBase(const components::ComponentConfig&,
                       const components::ComponentContext&);

  /// @brief Websocket handler code belongs here.
  virtual void Handle(WebSocketConnection& websocket,
                      server::request::RequestContext&) const = 0;

  /// @brief If any code is required for handshake validation,
  /// it goes here.
  virtual bool HandleHandshake(const server::http::HttpRequest&,
                               server::http::HttpResponse&,
                               server::request::RequestContext&) const {
    return true;
  }

  /// @cond
  void WriteMetrics(utils::statistics::Writer& writer) const;

  static yaml_config::Schema GetStaticConfigSchema();
  /// @endcond

 private:
  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext& context) const override;

  websocket::Config config_;
  mutable Statistics stats_;
  utils::statistics::Entry statistics_holder_;
};

}  // namespace server::websocket

USERVER_NAMESPACE_END
