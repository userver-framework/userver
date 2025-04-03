#pragma once

#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/middlewares/builtin.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/utils/impl/internal_tag.hpp>
#include <userver/utils/impl/transparent_hash.hpp>
#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
class AuthCheckerSettings;
}

namespace server::handlers::auth {

class AuthCheckerFactoryBase;

using AuthCheckerFactories = utils::impl::TransparentMap<std::string, utils::UniqueRef<AuthCheckerFactoryBase>>;

}  // namespace server::handlers::auth

namespace server::middlewares {

class Auth final : public HttpMiddlewareBase {
public:
    static constexpr std::string_view kName = builtin::kAuth;

    Auth(const handlers::auth::AuthCheckerFactories& factories, const handlers::HttpHandlerBase&);

private:
    void HandleRequest(http::HttpRequest& request, request::RequestContext& context) const override;

    bool CheckAuth(const http::HttpRequest& request, request::RequestContext& context) const;

    const handlers::HttpHandlerBase& handler_;
    std::vector<handlers::auth::AuthCheckerBasePtr> auth_checkers_;
};

class AuthFactory final : public HttpMiddlewareFactoryBase {
public:
    static constexpr std::string_view kName = Auth::kName;

    AuthFactory(const components::ComponentConfig&, const components::ComponentContext&);

    const handlers::auth::AuthCheckerFactoryBase& GetAuthCheckerFactory(std::string_view auth_type) const;

private:
    std::unique_ptr<HttpMiddlewareBase> Create(const handlers::HttpHandlerBase&, yaml_config::YamlConfig)
        const override;

    const handlers::auth::AuthCheckerFactories factories_;
};

}  // namespace server::middlewares

template <>
inline constexpr bool components::kHasValidate<server::middlewares::AuthFactory> = true;

template <>
inline constexpr auto components::kConfigFileMode<server::middlewares::AuthFactory> = ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
