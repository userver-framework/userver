#pragma once

#include <yandex/taxi/userver/components/manager.hpp>
#include <yandex/taxi/userver/server/handlers/http_handler_base.hpp>

namespace server {
namespace handlers {

class ServerMonitor : public HttpHandlerBase {
 public:
  ServerMonitor(const components::ComponentConfig& config,
                const components::ComponentContext& component_context);

  static constexpr const char* kName = "handler-server-monitor";

  virtual const std::string& HandlerName() const override;
  virtual std::string HandleRequestThrow(
      const http::HttpRequest& request,
      request::RequestContext&) const override;

 private:
  const components::Manager& components_manager_;
};

}  // namespace handlers
}  // namespace server
