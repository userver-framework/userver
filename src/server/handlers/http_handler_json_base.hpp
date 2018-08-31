#pragma once

#include <server/handlers/http_handler_base.hpp>

namespace server {
namespace handlers {

class HttpHandlerJsonBase : public HttpHandlerBase {
 public:
  HttpHandlerJsonBase(const components::ComponentConfig& config,
                      const components::ComponentContext& component_context);

  std::string HandleRequestThrow(
      const http::HttpRequest& request,
      request::RequestContext& context) const override final;

  virtual Json::Value HandleRequestJsonThrow(
      const http::HttpRequest& request, const Json::Value& request_json,
      request::RequestContext& context) const = 0;
};

}  // namespace handlers
}  // namespace server
