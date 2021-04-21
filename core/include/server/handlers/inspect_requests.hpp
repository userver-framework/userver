#pragma once

/// @file server/handlers/inspect_requests.hpp
/// @brief @copybrief server::handlers::InspectRequests

#include <components/manager.hpp>
#include <server/handlers/http_handler_json_base.hpp>

namespace server {

class RequestsView;

namespace handlers {
// clang-format off

/// @ingroup userver_http_handlers
///
/// @brief Handler that returns information about all in-flight requests.
///
/// The component has no service configuration except the
/// @ref userver_http_handlers "common handler options".
///
/// ## Configuration example:
///
/// @snippet server_settings/server_common_component_list_test.cpp  Sample handler inspect requests component config
///
/// ## Scheme
/// Provide an optional query parameter `body` to get the bodies of all the
/// in-flight requests.

// clang-format on
class InspectRequests final : public HttpHandlerJsonBase {
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
