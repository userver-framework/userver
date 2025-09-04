#include <userver/chaotic/openapi/middlewares/proxy_middleware.hpp>

#include <userver/clients/http/request.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

ProxyMiddleware::ProxyMiddleware(std::string&& proxy_url) : proxy_url_(std::move(proxy_url)) {}

void ProxyMiddleware::OnRequest(clients::http::Request& request) { request.proxy(proxy_url_); }

void ProxyMiddleware::OnResponse(clients::http::Response&) {}

std::string ProxyMiddleware::GetStaticConfigSchemaStr() {
    return R"(
type: object
description: Proxy middleware configuration
additionalProperties: false
properties:
    url:
        type: string
        description: Proxy server URL
)";
}

std::shared_ptr<client::Middleware> ProxyMiddlewareFactory::Create(const yaml_config::YamlConfig& config) {
    return std::make_shared<ProxyMiddleware>(config["url"].As<std::string>());
}

std::string ProxyMiddlewareFactory::GetStaticConfigSchemaStr() { return ProxyMiddleware::GetStaticConfigSchemaStr(); }

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END
