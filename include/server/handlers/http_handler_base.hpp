#pragma once

#include <string>

#include <components/manager.hpp>
#include <server/handlers/handler_base.hpp>
#include <server/http/http_request.hpp>
#include <server/http/http_response.hpp>
#include <server/request/http_server_settings_base_component.hpp>
#include <server/request/request_base.hpp>

namespace server {
namespace handlers {

class HttpHandlerBase : public HandlerBase {
 public:
  HttpHandlerBase(const components::ComponentConfig& config,
                  const components::ComponentContext& component_context,
                  bool is_monitor = false);

  ~HttpHandlerBase();

  virtual void HandleRequest(const request::RequestBase& request,
                             request::RequestContext& context) const
      noexcept override;
  virtual void OnRequestComplete(const request::RequestBase& request,
                                 request::RequestContext& context) const
      noexcept override;

  formats::json::Value GetMonitorData(
      components::MonitorVerbosity verbosity) const override;

  std::string GetMetricsPath() const override;

  virtual const std::string& HandlerName() const = 0;

 protected:
  virtual std::string HandleRequestThrow(
      const http::HttpRequest& request,
      request::RequestContext& context) const = 0;
  virtual void OnRequestCompleteThrow(
      const http::HttpRequest& /*request*/,
      request::RequestContext& /*context*/) const {}

 private:
  class Statistics;

  static formats::json::Value StatisticsToJson(const Statistics& stats);

 private:
  const components::HttpServerSettingsBase* http_server_settings_;

  std::unique_ptr<Statistics> statistics_;
};

}  // namespace handlers
}  // namespace server
