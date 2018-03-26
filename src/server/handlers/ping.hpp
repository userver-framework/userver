#pragma once

#include "http_handler_base.hpp"

namespace server {
namespace handlers {

class Ping : public HttpHandlerBase {
 public:
  Ping(const components::ComponentConfig& config,
       const components::ComponentContext& component_context);

  static constexpr const char* const kName = "handler-ping";

  const std::string& HandlerName() const override;
  std::string HandleRequestThrow(const server::http::HttpRequest& request,
                                 HandlerContext& context) const override;
};

}  // namespace handlers
}  // namespace server
