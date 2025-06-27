#pragma once

#include <userver/chaotic/openapi/client/middleware.hpp>
#include <userver/chaotic/openapi/client/middleware_factory.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

class FollowRedirectsMiddleware final : public client::Middleware {
public:
    explicit FollowRedirectsMiddleware(bool follow_redirects);

    void OnRequest(clients::http::Request& request) override;
    void OnResponse(clients::http::Response&) override;
    void ApplyFollowRedirects(bool follow_redirects);

    static std::string GetStaticConfigSchemaStr();

private:
    bool follow_redirects_;
};

class FollowRedirectsMiddlewareFactory final : public client::MiddlewareFactory {
public:
    FollowRedirectsMiddlewareFactory(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    )
        : client::MiddlewareFactory(config, context) {}

    static constexpr std::string_view kName = "chaotic-client-middleware-follow-redirects";

    std::shared_ptr<client::Middleware> Create(const USERVER_NAMESPACE::yaml_config::YamlConfig& config) override;
    std::string GetStaticConfigSchemaStr() override;
};

}  // namespace chaotic::openapi

USERVER_NAMESPACE_END

template <>
inline constexpr auto USERVER_NAMESPACE::components::kConfigFileMode<
    USERVER_NAMESPACE::chaotic::openapi::FollowRedirectsMiddlewareFactory> =
    USERVER_NAMESPACE::components::ConfigFileMode::kNotRequired;
