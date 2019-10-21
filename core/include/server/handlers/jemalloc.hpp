#pragma once

#include <logging/level.hpp>

#include <server/handlers/http_handler_base.hpp>

namespace server::handlers {

class Jemalloc final : public HttpHandlerBase {
 public:
  Jemalloc(const components::ComponentConfig& config,
           const components::ComponentContext& component_context);

  static constexpr const char* kName = "handler-jemalloc";

  const std::string& HandlerName() const override;
  std::string HandleRequestThrow(const http::HttpRequest& request,
                                 request::RequestContext&) const override;
};

}  // namespace server::handlers
