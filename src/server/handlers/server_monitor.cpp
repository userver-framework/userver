#include "server_monitor.hpp"

namespace server {
namespace handlers {

ServerMonitor::ServerMonitor(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context), monitor_(nullptr) {}

void ServerMonitor::SetMonitorPtr(const server::ServerMonitor* monitor) {
  monitor_ = monitor;
}

const std::string& ServerMonitor::HandlerName() const {
  static const std::string kHandlerName = kName;
  return kHandlerName;
}

std::string ServerMonitor::HandleRequestThrow(const http::HttpRequest& request,
                                              request::RequestContext&) const {
  if (!monitor_) throw std::runtime_error("monitor not set");
  return monitor_->GetJsonData(request);
}

}  // namespace handlers
}  // namespace server
