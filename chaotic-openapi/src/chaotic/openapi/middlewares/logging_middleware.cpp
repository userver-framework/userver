#include <userver/chaotic/openapi/middlewares/logging_middleware.hpp>

#include <userver/clients/http/request.hpp>
#include <userver/clients/http/response.hpp>
#include <userver/components/component_base.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/utils/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

LoggingMiddleware::LoggingMiddleware(logging::Level request_level, logging::Level response_level, size_t body_log_limit)
    : request_level_(request_level), response_level_(response_level), body_log_limit_(body_log_limit) {}

void LoggingMiddleware::OnRequest(clients::http::Request& request) {
    LOG(request_level_) << "Request body: " << utils::log::ToLimitedUtf8(request.GetData(), body_log_limit_)
                        << USERVER_NAMESPACE::logging::LogExtra{{"http_url", request.GetUrl()}};
}

void LoggingMiddleware::OnResponse(clients::http::Response& response) {
    LOG(response_level_) << "Response body: " << utils::log::ToLimitedUtf8(response.body_view(), body_log_limit_)
                         << USERVER_NAMESPACE::logging::LogExtra{{"meta_code", response.status_code()}};
}

std::string LoggingMiddleware::GetStaticConfigSchemaStr() {
    return R"(
type: object
description: Logging middleware configuration
additionalProperties: false
properties:
    request_level:
        type: string
        description: Log level for requests
        enum: [trace, debug, info, warning, error, critical, none]
        default: debug
    response_level:
        type: string
        description: Log level for responses
        enum: [trace, debug, info, warning, error, critical, none]
        default: debug
    body_limit:
        type: integer
        description: Maximum body size to log in bytes
        minimum: 0
        default: 1024
)";
}

std::shared_ptr<client::Middleware> LoggingMiddlewareFactory::Create(const yaml_config::YamlConfig& config) {
    const auto log_request_level = logging::LevelFromString(config["request_level"].As<std::string>("debug"));
    const auto log_response_level = logging::LevelFromString(config["response_level"].As<std::string>("debug"));
    return std::make_shared<LoggingMiddleware>(
        log_request_level, log_response_level, config["body_limit"].As<size_t>(1024)
    );
}

std::string LoggingMiddlewareFactory::GetStaticConfigSchemaStr() {
    return LoggingMiddleware::GetStaticConfigSchemaStr();
}

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END
