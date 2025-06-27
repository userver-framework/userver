#pragma once

#include <userver/chaotic/openapi/client/middleware.hpp>
#include <userver/chaotic/openapi/client/middleware_factory.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

class LoggingMiddleware final : public client::Middleware {
public:
    LoggingMiddleware(logging::Level request_level, logging::Level response_level, size_t body_log_limit);

    void OnRequest(clients::http::Request& request) override;
    void OnResponse(clients::http::Response& response) override;

    static std::string GetStaticConfigSchemaStr();

private:
    logging::Level request_level_;
    logging::Level response_level_;
    size_t body_log_limit_;
};

class LoggingMiddlewareFactory final : public client::MiddlewareFactory {
public:
    LoggingMiddlewareFactory(const components::ComponentConfig& config, const components::ComponentContext& context)
        : client::MiddlewareFactory(config, context) {}

    static constexpr std::string_view kName = "chaotic-client-middleware-logging";

    std::shared_ptr<client::Middleware> Create(const yaml_config::YamlConfig& config) override;
    std::string GetStaticConfigSchemaStr() override;
};

}  // namespace chaotic::openapi
   //

USERVER_NAMESPACE_END

template <>
inline constexpr auto
    USERVER_NAMESPACE::components::kConfigFileMode<USERVER_NAMESPACE::chaotic::openapi::LoggingMiddlewareFactory> =
        USERVER_NAMESPACE::components::ConfigFileMode::kNotRequired;
