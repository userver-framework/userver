#include <server/handlers/server_monitor.hpp>

#include <components/component_base.hpp>
#include <formats/json/serialize.hpp>

namespace server {
namespace handlers {

ServerMonitor::ServerMonitor(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context, /*is_monitor = */ true),
      statistics_storage_(
          component_context.FindComponent<components::StatisticsStorage>()) {}

const std::string& ServerMonitor::HandlerName() const {
  static const std::string kHandlerName = kName;
  return kHandlerName;
}

std::string ServerMonitor::HandleRequestThrow(const http::HttpRequest& request,
                                              request::RequestContext&) const {
  const auto prefix = request.GetArg("prefix");

  auto statistics_request = utils::statistics::StatisticsRequest();
  formats::json::ValueBuilder monitor_data =
      statistics_storage_.GetStorage().GetAsJson(prefix, statistics_request);

  return formats::json::ToString(monitor_data.ExtractValue());
}  // namespace server

}  // namespace handlers
}  // namespace server
