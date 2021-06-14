#pragma once

/// @file server/handlers/http_handler_base.hpp
/// @brief @copybrief server::handlers::HttpHandlerBase

#include <optional>
#include <string>
#include <vector>

#include <components/manager.hpp>
#include <logging/level.hpp>
#include <taxi_config/config_ptr.hpp>
#include <utils/token_bucket.hpp>

#include <server/handlers/auth/auth_checker_base.hpp>
#include <server/handlers/exceptions.hpp>
#include <server/handlers/formatted_error_data.hpp>
#include <server/handlers/handler_base.hpp>
#include <server/http/http_request.hpp>
#include <server/http/http_response.hpp>
#include <server/request/request_base.hpp>

// clang-format off

/// @defgroup userver_http_handlers Userver HTTP Handlers
///
/// @brief Handlers are @ref userver_components "components" that could be used
/// by components::Server to process the incomming requests.
///
/// All the HTTP handlers derive from server::handlers::HttpHandlerBase and
/// override its methods to provide functionality.
///
/// See @ref userver_components "Userver Components" for more information on
/// how to register components.
///
/// ## Configuration
/// All the handlers inherit configuration options from their base classes:
/// * server::handlers::HandlerBase
/// * server::handlers::HttpHandlerBase
///
/// All the components, including handlers, also have the
/// @ref userver_components "load-enabled" option.
///
/// ## Static configuration example:
/// Here's an example of the full config for the server::handlers::Ping handle.
/// @snippet components/common_server_component_list_test.cpp  Sample handler ping component config

// clang-format on

namespace components {
class StatisticsStorage;
}  // namespace components

/// @brief Most common \ref userver_http_handlers "userver HTTP handlers"
namespace server::handlers {

class HttpHandlerStatistics;
class HttpHandlerMethodStatistics;
class HttpHandlerStatisticsScope;

/// @ingroup userver_http_handlers
///
/// @brief Base class for all the
/// \ref userver_http_handlers "Userver HTTP Handlers"
///
/// ## Static options:
/// Inherits all the options from server::handlers::HandlerBase and adds the
/// following ones:
///
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// log-level | overrides log level for this handle | <no override>
class HttpHandlerBase : public HandlerBase {
 public:
  HttpHandlerBase(const components::ComponentConfig& config,
                  const components::ComponentContext& component_context,
                  bool is_monitor = false);

  ~HttpHandlerBase() override;

  void HandleRequest(request::RequestBase& request,
                     request::RequestContext& context) const override;
  void ReportMalformedRequest(request::RequestBase& request) const final;

  virtual const std::string& HandlerName() const = 0;

  const std::vector<http::HttpMethod>& GetAllowedMethods() const;

  HttpHandlerStatistics& GetRequestStatistics() const;

  /// Override it if you need a custom logging level for messages about finish
  /// of request handling for some http statuses.
  virtual logging::Level GetLogLevelForResponseStatus(
      http::HttpStatus status) const;

  virtual FormattedErrorData GetFormattedExternalErrorBody(
      const CustomHandlerException& exc) const;

  std::string GetResponseDataForLoggingChecked(
      const http::HttpRequest& request, request::RequestContext& context,
      const std::string& response_data) const;

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

  void CheckAuth(const http::HttpRequest& http_request,
                 request::RequestContext& context) const;

  void CheckRatelimit(const http::HttpRequest& http_request) const;

  void DecompressRequestBody(http::HttpRequest& http_request) const;

  static formats::json::ValueBuilder StatisticsToJson(
      const HttpHandlerMethodStatistics& stats);

  formats::json::ValueBuilder ExtendStatistics(
      const utils::statistics::StatisticsRequest&);

  formats::json::ValueBuilder FormatStatistics(const HttpHandlerStatistics&);

  void SetResponseAcceptEncoding(http::HttpResponse& response) const;
  void SetResponseServerHostname(http::HttpResponse& response) const;

  const taxi_config::Source config_source_;
  const std::vector<http::HttpMethod> allowed_methods_;
  components::StatisticsStorage& statistics_storage_;
  utils::statistics::Entry statistics_holder_;

  std::unique_ptr<HttpHandlerStatistics> handler_statistics_;
  std::unique_ptr<HttpHandlerStatistics> request_statistics_;
  std::vector<auth::AuthCheckerBasePtr> auth_checkers_;

  std::optional<logging::Level> log_level_;
  bool set_response_server_hostname_;
  mutable utils::TokenBucket rate_limit_;
};

}  // namespace server::handlers
