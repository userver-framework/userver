#pragma once

#include <components/manager.hpp>
#include <server/handlers/http_handler_json_base.hpp>

namespace server {

class RequestsView;

namespace handlers {

class InspectRequests : public HttpHandlerJsonBase {
 public:
  InspectRequests(const components::ComponentConfig& config,
                  const components::ComponentContext& component_context);

  static constexpr const char* kName = "handler-inspect-requests";

  const std::string& HandlerName() const override;

  formats::json::Value HandleRequestJsonThrow(
      const http::HttpRequest& request,
      const formats::json::Value& request_json,
      request::RequestContext& context) const override;

 private:
  RequestsView& view_;
};

}  // namespace handlers
}  // namespace server
