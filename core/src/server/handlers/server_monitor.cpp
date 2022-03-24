#include <userver/server/handlers/server_monitor.hpp>

#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/formats/json/serialize.hpp>

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
  formats::json::ValueBuilder monitor_data =
      statistics_storage_.GetAsJson(statistics_request);

  return formats::json::ToString(monitor_data.ExtractValue());
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
