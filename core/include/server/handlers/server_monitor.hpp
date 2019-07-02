#pragma once

#include <components/manager.hpp>
#include <components/statistics_storage.hpp>
#include <server/handlers/http_handler_base.hpp>

namespace server {
namespace handlers {

class ServerMonitor final : public HttpHandlerBase {
 public:
  ServerMonitor(const components::ComponentConfig& config,
                const components::ComponentContext& component_context);

  static constexpr const char* kName = "handler-server-monitor";

  const std::string& HandlerName() const override;
  std::string HandleRequestThrow(const http::HttpRequest& request,
                                 request::RequestContext&) const override;

  formats::json::Value GetEngineStats(
      utils::statistics::StatisticsRequest) const;

 private:
  std::string GetResponseDataForLogging(
      const http::HttpRequest& request, request::RequestContext& context,
      const std::string& response_data) const override;

  components::StatisticsStorage& statistics_storage_;
};

}  // namespace handlers
}  // namespace server
