#include <userver/server/handlers/ping.hpp>

#include <userver/server/handlers/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

Ping::Ping(const components::ComponentConfig& config,
           const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      components_(component_context) {}

const std::string& Ping::HandlerName() const {
  static const std::string kHandlerPingName = kName;

  return kHandlerPingName;
}

std::string Ping::HandleRequestThrow(
    const http::HttpRequest& /*request*/,
    request::RequestContext& /*context*/) const {
  if (components_.IsAnyComponentInFatalState()) {
    throw InternalServerError();
  }

  return {};
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
