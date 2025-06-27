#pragma once

#include <string>
#include <userver/chaotic/openapi/client/middleware.hpp>
#include <userver/chaotic/openapi/client/middleware_factory.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

class ProxyMiddleware final : public client::Middleware {
public:
    explicit ProxyMiddleware(std::string&& proxy_url);

    void OnRequest(clients::http::Request& request) override;
    void OnResponse(clients::http::Response&) override;

    static std::string GetStaticConfigSchemaStr();

private:
    std::string proxy_url_;
};

class ProxyMiddlewareFactory final : public client::MiddlewareFactory {
public:
    ProxyMiddlewareFactory(const components::ComponentConfig& config, const components::ComponentContext& context)
        : client::MiddlewareFactory(config, context) {}

    static constexpr std::string_view kName = "chaotic-client-middleware-proxy";

    std::shared_ptr<client::Middleware> Create(const yaml_config::YamlConfig& config) override;
    std::string GetStaticConfigSchemaStr() override;
};

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END

template <>
inline constexpr auto
    USERVER_NAMESPACE::components::kConfigFileMode<USERVER_NAMESPACE::chaotic::openapi::ProxyMiddlewareFactory> =
        USERVER_NAMESPACE::components::ConfigFileMode::kNotRequired;
