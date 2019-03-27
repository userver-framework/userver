#pragma once

#include <string>
#include <vector>

#include <components/manager.hpp>
#include <server/handlers/auth/auth_checker_base.hpp>
#include <server/handlers/handler_base.hpp>
#include <server/http/http_request.hpp>
#include <server/http/http_response.hpp>
#include <server/request/request_base.hpp>
#include <server_settings/http_server_settings_base_component.hpp>

namespace components {
class StatisticsStorage;
}  // namespace components

namespace server {
namespace handlers {

class HttpHandlerStatistics;
class HttpHandlerMethodStatistics;
class HttpHandlerStatisticsScope;

class HttpHandlerBase : public HandlerBase {
 public:
  HttpHandlerBase(const components::ComponentConfig& config,
                  const components::ComponentContext& component_context,
                  bool is_monitor = false);

  ~HttpHandlerBase();

  virtual void HandleRequest(const request::RequestBase& request,
                             request::RequestContext& context) const override;
  virtual void OnRequestComplete(
      const request::RequestBase& request,
      request::RequestContext& context) const override;

  virtual const std::string& HandlerName() const = 0;

  const std::vector<http::HttpMethod>& GetAllowedMethods() const;

  HttpHandlerStatistics& GetRequestStatistics() const;

 protected:
  [[noreturn]] void ThrowUnsupportedHttpMethod(
      const http::HttpRequest& request) const;

  virtual std::string HandleRequestThrow(
      const http::HttpRequest& request,
      request::RequestContext& context) const = 0;
  virtual void OnRequestCompleteThrow(
      const http::HttpRequest& /*request*/,
      request::RequestContext& /*context*/) const {}

  /* Override it to show per HTTP-method statistics besides statistics for all
   * methods */
  virtual bool IsMethodStatisticIncluded() const { return false; }

  // Override it if you want to disable auth checks in handler by some condition
  virtual bool NeedCheckAuth() const { return true; }

 private:
  void CheckAuth(const http::HttpRequest& http_request) const;

  static formats::json::ValueBuilder StatisticsToJson(
      const HttpHandlerMethodStatistics& stats);

  formats::json::ValueBuilder ExtendStatistics(
      const utils::statistics::StatisticsRequest&);

  formats::json::ValueBuilder FormatStatistics(const HttpHandlerStatistics&);

  const components::HttpServerSettingsBase& http_server_settings_;
  const std::vector<http::HttpMethod> allowed_methods_;
  components::StatisticsStorage& statistics_storage_;
  utils::statistics::Entry statistics_holder_;

  std::unique_ptr<HttpHandlerStatistics> handler_statistics_;
  std::unique_ptr<HttpHandlerStatistics> request_statistics_;
  std::vector<auth::AuthCheckerBasePtr> auth_checkers_;
};

}  // namespace handlers
}  // namespace server
