#pragma once

#include <chrono>

#include <userver/chaotic/openapi/client/middleware.hpp>
#include <userver/chaotic/openapi/client/middleware_factory.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

class QosMiddleware final : public client::Middleware {
public:
    QosMiddleware(std::chrono::milliseconds timeout, int retries);

    void OnRequest(clients::http::Request& request) override;
    void OnResponse(clients::http::Response&) override;
    void ApplyCommandControl(std::chrono::milliseconds timeout, int retries);

    static std::string GetStaticConfigSchemaStr();

private:
    std::chrono::milliseconds timeout_;
    int retries_;
};

class QosMiddlewareFactory final : public client::MiddlewareFactory {
public:
    QosMiddlewareFactory(const components::ComponentConfig& config, const components::ComponentContext& context)
        : client::MiddlewareFactory(config, context) {}

    static constexpr std::string_view kName = "chaotic-client-middleware-timeout-attempts";

    std::shared_ptr<client::Middleware> Create(const yaml_config::YamlConfig& config) override;
    std::string GetStaticConfigSchemaStr() override;
};

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END

template <>
inline constexpr auto
    USERVER_NAMESPACE::components::kConfigFileMode<USERVER_NAMESPACE::chaotic::openapi::QosMiddlewareFactory> =
        USERVER_NAMESPACE::components::ConfigFileMode::kNotRequired;
