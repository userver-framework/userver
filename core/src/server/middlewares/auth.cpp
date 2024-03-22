#include <server/middlewares/auth.hpp>

#include <server/handlers/auth/auth_checker.hpp>
#include <server/http/http_request_impl.hpp>

#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/handlers/auth/auth_checker_settings_component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_error.hpp>
#include <userver/tracing/scope_time.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

Auth::Auth(const components::ComponentContext& context,
           const handlers::HttpHandlerBase& handler)
    : handler_{handler},
      auth_checkers_{handlers::auth::CreateAuthCheckers(
          context, handler_.GetConfig(),
          context.FindComponent<components::AuthCheckerSettings>().Get())} {}

void Auth::HandleRequest(http::HttpRequest& request,
                         request::RequestContext& context) const {
  if (CheckAuth(request, context)) {
    Next(request, context);
  }
}

bool Auth::CheckAuth(const http::HttpRequest& request,
                     request::RequestContext& context) const {
  const auto scope_time =
      tracing::ScopeTime::CreateOptionalScopeTime("http_check_auth");
  if (!handler_.NeedCheckAuth()) {
    LOG_DEBUG() << "auth checks are disabled for current handler";
    return true;
  }

  try {
    handlers::auth::CheckAuth(auth_checkers_, request, context);
    return true;
  } catch (const handlers::CustomHandlerException& ex) {
    handler_.HandleCustomHandlerException(request, ex);
  } catch (const std::exception& ex) {
    handler_.HandleUnknownException(request, ex);
  }

  return false;
}

AuthFactory::AuthFactory(const components::ComponentConfig& config,
                         const components::ComponentContext& context)
    : HttpMiddlewareFactoryBase{config, context}, context_{context} {}

std::unique_ptr<HttpMiddlewareBase> AuthFactory::Create(
    const handlers::HttpHandlerBase& handler, yaml_config::YamlConfig) const {
  return std::make_unique<Auth>(context_, handler);
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
