#pragma once

#include <vector>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/handlers/auth/auth_checker_settings.hpp>
#include <userver/server/handlers/handler_config.hpp>
#include <userver/server/http/http_request.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

std::vector<AuthCheckerBasePtr> CreateAuthCheckers(
    const components::ComponentContext& component_context,
    const HandlerConfig& config, const AuthCheckerSettings& settings);

void CheckAuth(const std::vector<AuthCheckerBasePtr>& auth_checkers,
               const http::HttpRequest& http_request,
               request::RequestContext& context);

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
