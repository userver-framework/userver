#include "ping.hpp"

namespace server {
namespace handlers {

Ping::Ping(const components::ComponentConfig& config,
           const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context) {}

const std::string& Ping::HandlerName() const {
  static const std::string kHandlerPingName = Ping::kName;

  return kHandlerPingName;
}

std::string Ping::HandleRequestThrow(
    const http::HttpRequest& /*request*/,
    request::RequestContext& /*context*/) const {
  return {};
}

}  // namespace handlers
}  // namespace server
