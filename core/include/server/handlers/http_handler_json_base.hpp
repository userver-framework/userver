#pragma once

#include <server/handlers/http_handler_base.hpp>

namespace server {
namespace handlers {

/// Convenient base for handlers that accept requests with body in
/// json format.
class HttpHandlerJsonBase : public HttpHandlerBase {
 public:
  HttpHandlerJsonBase(const components::ComponentConfig& config,
                      const components::ComponentContext& component_context);

  std::string HandleRequestThrow(
      const http::HttpRequest& request,
      request::RequestContext& context) const override final;

  virtual formats::json::Value HandleRequestJsonThrow(
      const http::HttpRequest& request,
      const formats::json::Value& request_json,
      request::RequestContext& context) const = 0;
};

}  // namespace handlers
}  // namespace server
