#pragma once

#include <userver/server/middlewares/http_middleware_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

class HeadersPropagator final : public HttpMiddlewareBase {
public:
    explicit HeadersPropagator(const handlers::HttpHandlerBase&, std::vector<std::string> headers);

private:
    void HandleRequest(http::HttpRequest& request, request::RequestContext& context) const override;

    std::vector<std::string> headers_;
};

class HeadersPropagatorFactory final : public HttpMiddlewareFactoryBase {
public:
    static constexpr std::string_view kName = "headers-propagator";

    HeadersPropagatorFactory(const components::ComponentConfig&, const components::ComponentContext&);

    static yaml_config::Schema GetStaticConfigSchema();

private:
    std::unique_ptr<HttpMiddlewareBase>
    Create(const handlers::HttpHandlerBase&, yaml_config::YamlConfig middleware_config) const override;

    std::vector<std::string> headers_;
};

}  // namespace server::middlewares

template <>
inline constexpr bool components::kHasValidate<server::middlewares::HeadersPropagatorFactory> = true;

template <>
inline constexpr auto components::kConfigFileMode<server::middlewares::HeadersPropagatorFactory> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
