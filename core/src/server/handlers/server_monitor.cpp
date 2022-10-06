#include <userver/server/handlers/server_monitor.hpp>

#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/utils/statistics/prometheus.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/yaml_config/schema.hpp>

#include <utils/statistics/value_builder_helpers.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

ServerMonitor::ServerMonitor(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context, /*is_monitor = */ true),
      statistics_storage_(
          component_context.FindComponent<components::StatisticsStorage>()
              .GetStorage()) {}

std::string ServerMonitor::HandleRequestThrow(const http::HttpRequest& request,
                                              request::RequestContext&) const {
  utils::statistics::StatisticsRequest statistics_request;
  statistics_request.prefix = request.GetArg("prefix");
  const auto& format = request.GetArg("format");

  if (format == "prometheus") {
    // TODO: add common labels
    return utils::statistics::ToPrometheusFormat({}, statistics_storage_,
                                                 statistics_request);
  }

  formats::json::ValueBuilder monitor_data =
      statistics_storage_.GetAsJson(statistics_request);

  const auto json = monitor_data.ExtractValue();
  UASSERT(utils::statistics::AreAllMetricsNumbers(json));
  return formats::json::ToString(json);
}

std::string ServerMonitor::GetResponseDataForLogging(const http::HttpRequest&,
                                                     request::RequestContext&,
                                                     const std::string&) const {
  // Useless data for logs, no need to duplicate metrics in logs
  return "<statistics data>";
}

yaml_config::Schema ServerMonitor::GetStaticConfigSchema() {
  auto schema = HttpHandlerBase::GetStaticConfigSchema();
  schema.UpdateDescription("handler-server-monitor config");
  return schema;
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
