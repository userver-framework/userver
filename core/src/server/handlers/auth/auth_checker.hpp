#pragma once

#include <vector>

#include <components/component_context.hpp>
#include <server/handlers/auth/auth_checker_base.hpp>
#include <server/handlers/auth/auth_checker_settings.hpp>
#include <server/handlers/handler_config.hpp>
#include <server/http/http_request.hpp>

namespace server {
namespace handlers {
namespace auth {

std::vector<AuthCheckerBasePtr> CreateAuthCheckers(
    const components::ComponentContext& component_context,
    const HandlerConfig& config, const AuthCheckerSettings& settings);

void CheckAuth(const std::vector<AuthCheckerBasePtr>& auth_checkers,
               const http::HttpRequest& http_request,
               request::RequestContext& context);

}  // namespace auth
}  // namespace handlers
}  // namespace server
