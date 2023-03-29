#include <userver/server/handlers/handler_base.hpp>

#include <server/server_config.hpp>
#include <userver/components/component.hpp>
#include <userver/server/component.hpp>
#include <userver/server/handlers/handler_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

HandlerBase::HandlerBase(const components::ComponentConfig& config,
                         const components::ComponentContext& context,
                         bool is_monitor)
    : LoggableComponentBase(config, context),
      is_monitor_(config["monitor-handler"].As<bool>(is_monitor)),
      config_(ParseHandlerConfigsWithDefaults(
          config,
          context.FindComponent<components::Server>().GetServer().GetConfig(),
          is_monitor_)) {}

const HandlerConfig& HandlerBase::GetConfig() const { return config_; }

yaml_config::Schema HandlerBase::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Base class for the HTTP request handlers.
additionalProperties: false
properties:
    path:
        type: string
        description: if a request matches this path wildcard then process it by handler
    as_fallback:
        type: string
        description: set to "implicit-http-options" and do not specify a path if this handler processes the OPTIONS requests for paths that do not process OPTIONS method
        enum:
          - implicit-http-options
          - unknown
    task_processor:
        type: string
        description: a task processor to execute the requests
    method:
        type: string
        description: comma-separated list of allowed methods
    max_request_size:
        type: integer
        description: max size of the whole request
        defaultDescription: 1024 * 1024
    max_headers_size:
        type: integer
        description: max request headers size of empty to do not limit
    parse_args_from_body:
        type: boolean
        description: optional field to parse request according to x-www-form-urlencoded rules and make parameters accessible as query parameters
        defaultDescription: false
    auth:
        type: object
        description: server::handlers::auth::HandlerAuthConfig authorization config
        additionalProperties: true
        properties:
            type:
                type: string
                description: auth type
            types:
                type: array
                description: list of auth types
                items:
                    type: string
                    description: auth type
    url_trailing_slash:
        type: string
        description: "'both' to treat URLs with and without a trailing slash as equal, 'strict-match' otherwise"
        defaultDescription: 'both'
        enum:
          - both
          - strict-match
    max_requests_in_flight:
        type: integer
        description: integer to limit max pending requests to this handler
        defaultDescription: <no limit>
    request_body_size_log_limit:
        type: integer
        description: trim request to this size before logging
        defaultDescription: 512
    response_data_size_log_limit:
        type: integer
        description: trim responses to this size before logging
        defaultDescription: 512
    max_requests_per_second:
        type: integer
        description: integer to limit RPS to this handler
        defaultDescription: <no limit>
    decompress_request:
        type: boolean
        description: allow decompression of the requests
        defaultDescription: false
    throttling_enabled:
        type: boolean
        description: allow throttling of the requests by components::Server , for more info see its `max_response_size_in_flight` and `requests_queue_size_threshold` options
        defaultDescription: true
    set-response-server-hostname:
        type: boolean
        description: set to true to add the `X-YaTaxi-Server-Hostname` header with instance name, set to false to not add the header
        defaultDescription: <takes the value from components::Server config>
    response-body-stream:
        type: boolean
        description: TODO
        defaultDescription: false
    monitor-handler:
        type: boolean
        description: overrides the in-code `is_monitor` flag that makes the handler run either on 'server.listener' or on 'server.listener-monitor'
        defaultDescription: uses in-code flag value
    set_tracing_headers:
        type: boolean
        description: whether to set http tracing headers (X-YaTraceId, X-YaSpanId, X-RequestId)
        defaultDescription: true
)");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
