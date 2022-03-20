#include <userver/server/handlers/handler_base.hpp>

#include <userver/components/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

HandlerBase::HandlerBase(const components::ComponentConfig& config,
                         const components::ComponentContext& context,
                         bool is_monitor)
    : LoggableComponentBase(config, context),
      config_(config.As<HandlerConfig>()),
      is_monitor_(is_monitor) {}

const HandlerConfig& HandlerBase::GetConfig() const { return config_; }

yaml_config::Schema HandlerBase::GetStaticConfigSchema() {
  return yaml_config::Schema(R"(
type: object
description: handler base config
additionalProperties: false
properties:
    path:
        type: string
        description: if a request matches this path wildcard then process it by handler
    as_fallback:
        type: string
        description: set to "implicit-http-options" and do not specify a path if this handler processes the OPTIONS requests for paths that do not process OPTIONS method
    task_processor:
        type: string
        description: a task processor to execute the requests
    method:
        type: string
        description: comma-separated list of allowed methods
    max_url_size:
        type: integer
        description: max request path/URL size or empty to not limit
    max_request_size:
        type: integer
        description: max size of the whole request
        defaultDescription: 1024 * 1024
    max_headers_size:
        type: integer
        description: max request headers size of empy to do not limit
    parse_args_from_body:
        type: boolean
        description: optional field to parse request according to x-www-form-urlencoded rules and make parameters accessible as query parameters
        defaultDescription: false
    auth:
        type: object
        description: server::handlers::auth::HandlerAuthConfig authorization config
        additionalProperties: false
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
)");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
