#pragma once

/// @file userver/server/handlers/server_monitor.hpp
/// @brief @copybrief server::handlers::ServerMonitor

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/statistics/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

// clang-format off

/// @ingroup userver_components userver_http_handlers
///
/// @brief Handler that returns statistics data.
///
/// The component has no service configuration except the
/// @ref userver_http_handlers "common handler options".
///
/// ## Static configuration example:
///
/// @snippet components/common_server_component_list_test.cpp  Sample handler server monitor component config
///
/// ## Scheme
/// Accepts a path argument `prefix` and pass it to
/// utils::statistics::Storage::GetAsJson()

// clang-format on
class ServerMonitor final : public HttpHandlerBase {
 public:
  ServerMonitor(const components::ComponentConfig& config,
                const components::ComponentContext& component_context);

  static constexpr std::string_view kName = "handler-server-monitor";

  std::string HandleRequestThrow(const http::HttpRequest& request,
                                 request::RequestContext&) const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  std::string GetResponseDataForLogging(
      const http::HttpRequest& request, request::RequestContext& context,
      const std::string& response_data) const override;

  utils::statistics::Storage& statistics_storage_;
};

}  // namespace server::handlers

template <>
inline constexpr bool
    components::kHasValidate<server::handlers::ServerMonitor> = true;

USERVER_NAMESPACE_END
