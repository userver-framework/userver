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
/// ## Static options:
/// Inherits all the options from server::handlers::HttpHandlerBase
/// @ref userver_http_handlers
/// and adds the following ones:
///
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// warmup-time-secs | how much time it needs to warmup the server | 0
class Ping final : public HttpHandlerBase {
 public:
  Ping(const components::ComponentConfig& config,
       const components::ComponentContext& component_context);

  /// @ingroup userver_component_names
  /// @brief The default name of server::handlers::Ping
  static constexpr std::string_view kName = "handler-ping";

  std::string HandleRequestThrow(
      const http::HttpRequest& request,
      request::RequestContext& context) const override;

  void OnAllComponentsLoaded() override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  void AppendWeightHeaders(http::HttpResponse&) const;

  const components::ComponentContext& components_;

  std::chrono::steady_clock::time_point load_time_{};
  std::chrono::seconds awacs_weight_warmup_time_{60};
};

}  // namespace server::handlers

template <>
inline constexpr bool components::kHasValidate<server::handlers::Ping> = true;

USERVER_NAMESPACE_END
