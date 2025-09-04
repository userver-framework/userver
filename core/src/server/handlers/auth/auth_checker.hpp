#pragma once

#include <vector>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/handlers/handler_config.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

class AuthCheckerFactoryBase;

using AuthCheckerFactories = utils::impl::TransparentMap<std::string, utils::UniqueRef<AuthCheckerFactoryBase>>;

AuthCheckerFactories CreateAuthCheckerFactories(const components::ComponentContext&);

std::vector<AuthCheckerBasePtr> CreateAuthCheckers(const AuthCheckerFactories& factories, const HandlerConfig& config);

void CheckAuth(
    const std::vector<AuthCheckerBasePtr>& auth_checkers,
    const http::HttpRequest& http_request,
    request::RequestContext& context
);

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
