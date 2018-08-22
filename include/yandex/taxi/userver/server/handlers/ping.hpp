#pragma once

#include <yandex/taxi/userver/server/handlers/http_handler_base.hpp>

namespace server {
namespace handlers {

class Ping : public HttpHandlerBase {
 public:
  Ping(const components::ComponentConfig& config,
       const components::ComponentContext& component_context);

  static constexpr const char* kName = "handler-ping";

  const std::string& HandlerName() const override;
  std::string HandleRequestThrow(
      const http::HttpRequest& request,
      request::RequestContext& context) const override;
};

}  // namespace handlers
}  // namespace server
