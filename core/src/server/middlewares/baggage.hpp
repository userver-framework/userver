#pragma once

#include <userver/server/middlewares/http_middleware_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

class Baggage final : public HttpMiddlewareBase {
 public:
  static constexpr std::string_view kName{"userver-baggage-middleware"};

  explicit Baggage(const handlers::HttpHandlerBase&);

 private:
  void HandleRequest(http::HttpRequest& request,
                     request::RequestContext& context) const override;
};

using BaggageFactory = SimpleHttpMiddlewareFactory<Baggage>;

}  // namespace server::middlewares

USERVER_NAMESPACE_END
