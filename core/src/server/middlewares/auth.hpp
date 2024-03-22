#pragma once

#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
class AuthCheckerSettings;
}

namespace server::middlewares {

class Auth final : public HttpMiddlewareBase {
 public:
  static constexpr std::string_view kName{"userver-auth-middleware"};

  Auth(const components::ComponentContext&, const handlers::HttpHandlerBase&);

 private:
  void HandleRequest(http::HttpRequest& request,
                     request::RequestContext& context) const override;

  bool CheckAuth(const http::HttpRequest& request,
                 request::RequestContext& context) const;

  const handlers::HttpHandlerBase& handler_;
  std::vector<handlers::auth::AuthCheckerBasePtr> auth_checkers_;
};

class AuthFactory final : public HttpMiddlewareFactoryBase {
 public:
  static constexpr std::string_view kName = Auth::kName;

  AuthFactory(const components::ComponentConfig&,
              const components::ComponentContext&);

 private:
  std::unique_ptr<HttpMiddlewareBase> Create(
      const handlers::HttpHandlerBase&, yaml_config::YamlConfig) const override;

  const components::ComponentContext& context_;
};

}  // namespace server::middlewares

template <>
inline constexpr bool
    components::kHasValidate<server::middlewares::AuthFactory> = true;

template <>
inline constexpr auto
    components::kConfigFileMode<server::middlewares::AuthFactory> =
        ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
