#include <userver/chaotic/openapi/middlewares/follow_redirects_middleware.hpp>

#include <userver/clients/http/request.hpp>
#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

FollowRedirectsMiddleware::FollowRedirectsMiddleware(bool follow_redirects) : follow_redirects_(follow_redirects) {}

void FollowRedirectsMiddleware::OnRequest(clients::http::Request& request) {
    request.follow_redirects(follow_redirects_);
}

void FollowRedirectsMiddleware::OnResponse(clients::http::Response&) {}

void FollowRedirectsMiddleware::ApplyFollowRedirects(bool follow_redirects) { follow_redirects_ = follow_redirects; }

std::string FollowRedirectsMiddleware::GetStaticConfigSchemaStr() {
    return R"(
type: object
description: Follow redirects middleware configuration
additionalProperties: false
properties:
    enabled:
        type: boolean
        description: Whether to follow HTTP redirects
)";
}

std::shared_ptr<client::Middleware> FollowRedirectsMiddlewareFactory::Create(const yaml_config::YamlConfig& config) {
    return std::make_shared<FollowRedirectsMiddleware>(config["enabled"].As<bool>(true));
}

std::string FollowRedirectsMiddlewareFactory::GetStaticConfigSchemaStr() {
    return FollowRedirectsMiddleware::GetStaticConfigSchemaStr();
}

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END
