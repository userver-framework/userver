#pragma once

#include <userver/components/state.hpp>
#include <userver/server/middlewares/builtin.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>
USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

class GracefulShutdownHeaders final : public HttpMiddlewareBase {
public:
    explicit GracefulShutdownHeaders(const handlers::HttpHandlerBase&, const components::State& state);

private:
    void HandleRequest(http::HttpRequest& request, request::RequestContext& context) const override;

    const components::State state_;
};

class GracefulShutdownHeadersFactory final : public HttpMiddlewareFactoryBase {
public:
    static constexpr std::string_view kName = builtin::kGracefulShutdownHeaders;

    GracefulShutdownHeadersFactory(const components::ComponentConfig&, const components::ComponentContext& context);

private:
    std::unique_ptr<HttpMiddlewareBase> Create(
        const handlers::HttpHandlerBase&,
        yaml_config::YamlConfig middleware_config
    ) const override;

    const components::State state_;
};

}  // namespace server::middlewares

template <>
inline constexpr bool components::kHasValidate<server::middlewares::GracefulShutdownHeadersFactory> = true;

template <>
inline constexpr auto
    components::kConfigFileMode<server::middlewares::GracefulShutdownHeadersFactory> = ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
