#pragma once

/// @file userver/server/handlers/http_handler_base.hpp
/// @brief @copybrief server::handlers::HttpHandlerBase

#include <optional>
#include <string>
#include <vector>

#include <userver/dynamic_config/source.hpp>
#include <userver/logging/level.hpp>
#include <userver/utils/statistics/entry.hpp>
#include <userver/utils/token_bucket.hpp>

#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/formatted_error_data.hpp>
#include <userver/server/handlers/handler_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/server/http/http_response_body_stream_fwd.hpp>
#include <userver/server/request/request_base.hpp>
#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
class TracingManagerBase;
}  // namespace tracing

/// @brief Most common \ref userver_http_handlers "userver HTTP handlers"
namespace server::handlers {

class HttpHandlerStatistics;
class HttpRequestStatistics;
class HttpHandlerMethodStatistics;
class HttpHandlerStatisticsScope;

// clang-format off

/// @ingroup userver_components userver_http_handlers userver_base_classes
///
/// @brief Base class for all the
/// \ref userver_http_handlers "Userver HTTP Handlers".
///
/// ## Static options:
/// Inherits all the options from server::handlers::HandlerBase and adds the
/// following ones:
///
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// log-level | overrides log level for this handle | <no override>
///
/// ## Example usage:
///
/// @snippet samples/hello_service/hello_service.cpp Hello service sample - component

// clang-format on

class HttpHandlerBase : public HandlerBase {
 public:
  HttpHandlerBase(const components::ComponentConfig& config,
                  const components::ComponentContext& component_context,
                  bool is_monitor = false);

  ~HttpHandlerBase() override;

  void HandleRequest(request::RequestBase& request,
                     request::RequestContext& context) const override;

  void ReportMalformedRequest(request::RequestBase& request) const final;

  virtual const std::string& HandlerName() const;

  const std::vector<http::HttpMethod>& GetAllowedMethods() const;

  /// @cond
  // For internal use only.
  HttpHandlerStatistics& GetHandlerStatistics() const;

  // For internal use only.
  HttpRequestStatistics& GetRequestStatistics() const;
  /// @endcond

  /// Override it if you need a custom logging level for messages about finish
  /// of request handling for some http statuses.
  virtual logging::Level GetLogLevelForResponseStatus(
      http::HttpStatus status) const;

  virtual FormattedErrorData GetFormattedExternalErrorBody(
      const CustomHandlerException& exc) const;

  std::string GetResponseDataForLoggingChecked(
      const http::HttpRequest& request, request::RequestContext& context,
      const std::string& response_data) const;

  static yaml_config::Schema GetStaticConfigSchema();

 protected:
  [[noreturn]] void ThrowUnsupportedHttpMethod(
      const http::HttpRequest& request) const;

  /// The core method for HTTP request handling.
  /// `request` arg contains HTTP headers, full body, etc.
  /// The method should return response body.
  /// @note It is used only if IsStreamed() returned `false`.
  virtual std::string HandleRequestThrow(
      const http::HttpRequest& request, request::RequestContext& context) const;

  virtual void OnRequestCompleteThrow(
      const http::HttpRequest& /*request*/,
      request::RequestContext& /*context*/) const {}

  /// The core method for HTTP request handling.
  /// `request` arg contains HTTP headers, full body, etc.
  /// The response body is passed in parts to `ResponseBodyStream`.
  /// Stream transmission is useful when:
  /// 1) The body size is unknown beforehand.
  /// 2) The client may take advantage of early body transmission
  ///    (e.g. a Web Browser may start rendering the HTML page
  ///     or downloading dependant resources).
  /// 3) The body size is huge and we want to have only a part of it
  ///    in memory.
  /// @note It is used only if IsStreamed() returned `true`.
  virtual void HandleStreamRequest(const server::http::HttpRequest&,
                                   server::request::RequestContext&,
                                   server::http::ResponseBodyStream&) const;

  /// If IsStreamed() returns `true`, call HandleStreamRequest()
  /// for request handling, HandleRequestThrow() is not called.
  /// If it returns `false`, HandleRequestThrow() is called instead,
  /// and HandleStreamRequest() is not called.
  /// @note The default implementation returns the cached value of
  /// "response-body-streamed" value from static config.
  virtual bool IsStreamed() const { return is_body_streamed_; }

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

  /// For internal use. You don't need to override it. This method is overridden
  /// in format-specific base handlers.
  virtual void ParseRequestData(const http::HttpRequest&,
                                request::RequestContext&) const {}

  virtual std::string GetMetaType(const http::HttpRequest&) const;

 private:
  void HandleRequestStream(const http::HttpRequest& http_request,
                           request::RequestContext& context) const;

  std::string GetRequestBodyForLoggingChecked(
      const http::HttpRequest& request, request::RequestContext& context,
      const std::string& request_body) const;

  tracing::Span MakeSpan(const http::HttpRequest& http_request,
                         const std::string& meta_type) const;

  void CheckAuth(const http::HttpRequest& http_request,
                 request::RequestContext& context) const;

  void CheckRatelimit(const http::HttpRequest& http_request) const;

  void DecompressRequestBody(http::HttpRequest& http_request) const;

  template <typename HttpStatistics>
  void FormatStatistics(utils::statistics::Writer result,
                        const HttpStatistics& stats);

  void SetResponseAcceptEncoding(http::HttpResponse& response) const;
  void SetResponseServerHostname(http::HttpResponse& response) const;

  const dynamic_config::Source config_source_;
  const std::vector<http::HttpMethod> allowed_methods_;
  const std::string handler_name_;
  utils::statistics::Entry statistics_holder_;
  const tracing::TracingManagerBase& tracing_manager_;

  std::unique_ptr<HttpHandlerStatistics> handler_statistics_;
  std::unique_ptr<HttpRequestStatistics> request_statistics_;
  std::vector<auth::AuthCheckerBasePtr> auth_checkers_;

  std::optional<logging::Level> log_level_;
  bool set_response_server_hostname_;
  mutable utils::TokenBucket rate_limit_;
  bool is_body_streamed_;
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
