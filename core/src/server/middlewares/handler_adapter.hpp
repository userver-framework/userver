#pragma once

#include <userver/server/middlewares/http_middleware_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

// This is a middleware-pipeline terminating middleware: it's always the last
// one in a pipeline and calls the handlers 'HandleRequest'.
class HandlerAdapter final : public HttpMiddlewareBase {
 public:
  static constexpr std::string_view kName{"userver-handler-adapter-middleware"};

  explicit HandlerAdapter(const handlers::HttpHandlerBase&);

 private:
  void HandleRequest(http::HttpRequest& request,
                     request::RequestContext& context) const override;

  void ParseRequestData(const http::HttpRequest& request,
                        request::RequestContext& context) const;

  void LogRequest(const http::HttpRequest& request,
                  request::RequestContext& context) const;

  const handlers::HttpHandlerBase& handler_;
};

using HandlerAdapterFactory = SimpleHttpMiddlewareFactory<HandlerAdapter>;

}  // namespace server::middlewares

USERVER_NAMESPACE_END
