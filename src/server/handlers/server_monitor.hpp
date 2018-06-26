#pragma once

#include <server/server_monitor.hpp>

#include "http_handler_base.hpp"

namespace server {
namespace handlers {

class ServerMonitor : public HttpHandlerBase {
 public:
  ServerMonitor(const components::ComponentConfig& config,
                const components::ComponentContext& component_context);

  static constexpr const char* const kName = "handler-server-monitor";

  void SetMonitorPtr(const server::ServerMonitor* monitor);

  virtual const std::string& HandlerName() const override;
  virtual std::string HandleRequestThrow(
      const http::HttpRequest& request,
      request::RequestContext&) const override;

  bool IsMonitor() const override { return true; }

 private:
  const server::ServerMonitor* monitor_;
};

}  // namespace handlers
}  // namespace server
