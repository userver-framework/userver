#pragma once

/// @file userver/server/handlers/handler_base.hpp
/// @brief @copybrief server::handlers::HandlerBase

#include <userver/components/loggable_component_base.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/handlers/handler_config.hpp>
#include <userver/server/request/request_base.hpp>
#include <userver/server/request/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

// clang-format off

/// @brief Base class for the request handlers.
///
/// Each handler has an associated path and invoked only for the requests for
/// that path.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// path | if a request matches this path wildcard then process it by handler | -
/// as_fallback | set to "implicit-http-options" and do not specify a path if this handler processes the OPTIONS requests for paths that do not process OPTIONS method | -
/// task_processor | a task processor to execute the requests | -
/// method | comma-separated list of allowed methods | -
/// max_request_size | max size of the whole request | 1024 * 1024
/// max_headers_size | max request headers size | 65536
/// parse_args_from_body | optional field to parse request according to x-www-form-urlencoded rules and make parameters accessible as query parameters | false
/// auth | server::handlers::auth::HandlerAuthConfig authorization config | -
/// url_trailing_slash | 'both' to treat URLs with and without a trailing slash as equal, 'strict-match' otherwise | 'both'
/// max_requests_in_flight | integer to limit max pending requests to this handler | <no limit>
/// request_body_size_log_limit | trim request to this size before logging | 512
/// response_data_size_log_limit | trim responses to this size before logging | 512
/// max_requests_per_second | integer to limit RPS to this handler | <no limit>
/// decompress_request | allow decompression of the requests | false
/// throttling_enabled | allow throttling of the requests by components::Server , for more info see its `max_response_size_in_flight` and `requests_queue_size_threshold` options | true
/// set-response-server-hostname | set to true to add the `X-YaTaxi-Server-Hostname` header with instance name, set to false to not add the header | <takes the value from components::Server config>
/// monitor-handler | Overrides the in-code `is_monitor` flag that makes the handler run either on `server.listener` or on `server.listener-monitor` | --
/// set_tracing_headers | whether to set http tracing headers (X-YaTraceId, X-YaSpanId, X-RequestId) | true

// clang-format on
class HandlerBase : public components::LoggableComponentBase {
 public:
  HandlerBase(const components::ComponentConfig& config,
              const components::ComponentContext& component_context,
              bool is_monitor = false);
  ~HandlerBase() noexcept override = default;

  /// Parses request, executes processing routines, and fills response
  /// accordingly. Does not throw.
  virtual void HandleRequest(request::RequestBase& request,
                             request::RequestContext& context) const = 0;

  /// Produces response to a request unrecognized by the protocol based on
  /// provided generic response. Does not throw.
  virtual void ReportMalformedRequest(request::RequestBase&) const {}

  /// Returns whether this is a monitoring handler.
  bool IsMonitor() const { return is_monitor_; }

  /// Returns handler config.
  const HandlerConfig& GetConfig() const;

  static yaml_config::Schema GetStaticConfigSchema();

 protected:
  // Pull the type names in the handler's scope to shorten throwing code
  using HandlerErrorCode = handlers::HandlerErrorCode;
  using InternalMessage = handlers::InternalMessage;
  using ExternalBody = handlers::ExternalBody;

  using ClientError = handlers::ClientError;
  using InternalServerError = handlers::InternalServerError;

 private:
  bool is_monitor_;
  HandlerConfig config_;
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
