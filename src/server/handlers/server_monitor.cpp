#include "server_monitor.hpp"

#include <json/writer.h>

namespace server {
namespace handlers {

ServerMonitor::ServerMonitor(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      components_manager_(component_context.GetManager()) {}

const std::string& ServerMonitor::HandlerName() const {
  static const std::string kHandlerName = kName;
  return kHandlerName;
}

std::string ServerMonitor::HandleRequestThrow(const http::HttpRequest& request,
                                              request::RequestContext&) const {
  using Verbosity = components::MonitorVerbosity;
  const auto verbosity =
      request.GetArg("full") == "1" ? Verbosity::kFull : Verbosity::kTerse;
  return Json::FastWriter().write(
      components_manager_.GetMonitorData(verbosity));
}

}  // namespace handlers
}  // namespace server
