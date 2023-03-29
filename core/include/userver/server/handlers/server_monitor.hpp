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
/// Additionally to the
/// @ref userver_http_handlers "common handler options" the component has
/// 'common-labels' option that should be a map of label name to label value.
/// Items of the map are added to each metric.
///
/// ## Static configuration example:
///
/// @snippet components/common_server_component_list_test.cpp  Sample handler server monitor component config
///
/// ## Scheme
///
/// Accepts a path arguments 'format', 'labels', 'path' and 'prefix':
/// * format - "prometheus", "prometheus-untyped", "graphite",
///   "json", "solomon" and internal (default) format is
///   supported. For more info see the
///   documentation for utils::statistics::ToPrometheusFormat,
///   utils::statistics::ToPrometheusFormatUntyped,
///   utils::statistics::ToGraphiteFormat, utils::statistics::ToJsonFormat,
///   utils::statistics::ToSolomonFormat.
/// * labels - filter out metrics without the provided labels. Parameter should
///   be a JSON dictionary in the form '{"label1":"value1", "label2":"value2"}'.
/// * path - return metrics on for the following path
/// * prefix - return metrics whose path starts from the specified prefix.

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

  using CommonLabels = std::unordered_map<std::string, std::string>;
  const CommonLabels common_labels_;
};

}  // namespace server::handlers

template <>
inline constexpr bool
    components::kHasValidate<server::handlers::ServerMonitor> = true;

USERVER_NAMESPACE_END
