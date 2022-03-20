#pragma once

/// @file userver/server/handlers/ping.hpp
/// @brief @copybrief server::handlers::Ping

#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

/// @ingroup userver_components userver_http_handlers
///
/// @brief Handler that returns HTTP 200 if the service is OK and able to
/// process requests.
///
/// Uses components::ComponentContext::IsAnyComponentInFatalState() to detect
/// fatal state (can not process requests).
///
/// The component has no service configuration except the
/// @ref userver_http_handlers "common handler options".
class Ping final : public HttpHandlerBase {
 public:
  Ping(const components::ComponentConfig& config,
       const components::ComponentContext& component_context);

  static constexpr std::string_view kName = "handler-ping";

  std::string HandleRequestThrow(
      const http::HttpRequest& request,
      request::RequestContext& context) const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  const components::ComponentContext& components_;
};

}  // namespace server::handlers

template <>
inline constexpr bool components::kHasValidate<server::handlers::Ping> = true;

USERVER_NAMESPACE_END
