#pragma once

/// @file server/handlers/http_handler_base.hpp

#include <string>
#include <vector>

#include <components/manager.hpp>
#include <logging/level.hpp>
#include <server/handlers/auth/auth_checker_base.hpp>
#include <server/handlers/formatted_error_data.hpp>
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

  ~HttpHandlerBase() override;

  void HandleRequest(const request::RequestBase& request,
                     request::RequestContext& context) const override;
  void ReportMalformedRequest(const request::RequestBase& request) const final;

  virtual const std::string& HandlerName() const = 0;

  const std::vector<http::HttpMethod>& GetAllowedMethods() const;

  HttpHandlerStatistics& GetRequestStatistics() const;

  /// Override it if you need a custom logging level for messages about finish
  /// of request handling for some http statuses.
  virtual logging::Level GetLogLevelForResponseStatus(
      http::HttpStatus status) const;

  virtual FormattedErrorData GetFormattedExternalErrorBody(
      http::HttpStatus status, const std::string& error_code,
      std::string external_error_body) const;

 protected:
  [[noreturn]] void ThrowUnsupportedHttpMethod(
      const http::HttpRequest& request) const;

  virtual std::string HandleRequestThrow(
      const http::HttpRequest& request,
      request::RequestContext& context) const = 0;
  virtual void OnRequestCompleteThrow(
      const http::HttpRequest& /*request*/,
      request::RequestContext& /*context*/) const {}

  /// Override it to show per HTTP-method statistics besides statistics for all
  /// methods
  virtual bool IsMethodStatisticIncluded() const { return false; }

  /// Override it if you want to disable auth checks in handler by some
  /// condition
  virtual bool NeedCheckAuth() const { return true; }

  /// Override it if you need a custom request body logging.
  virtual std::string GetRequestBodyForLogging(
      const http::HttpRequest& request, request::RequestContext& context,
      const std::string& request_body) const;

  /// Override it if you need a custom response data logging.
  virtual std::string GetResponseDataForLogging(
      const http::HttpRequest& request, request::RequestContext& context,
      const std::string& response_data) const;

  /// For internal use. You don't need to override it. This method is overriden
  /// in format-specific base handlers.
  virtual void ParseRequestData(const http::HttpRequest&,
                                request::RequestContext&) const {}

  virtual std::string GetMetaType(const http::HttpRequest&) const;

 private:
  std::string GetRequestBodyForLoggingChecked(
      const http::HttpRequest& request, request::RequestContext& context,
      const std::string& request_body) const;

  std::string GetResponseDataForLoggingChecked(
      const http::HttpRequest& request, request::RequestContext& context,
      const std::string& response_data) const;

  void CheckAuth(const http::HttpRequest& http_request,
                 request::RequestContext& context) const;

  void CheckRatelimit(const http::HttpRequest& http_request) const;

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

  boost::optional<logging::Level> log_level_;
};

}  // namespace handlers
}  // namespace server
