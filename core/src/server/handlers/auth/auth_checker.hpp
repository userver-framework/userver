#pragma once

#include <components/component_context.hpp>
#include <server/handlers/auth/auth_checker_base.hpp>
#include <server/handlers/auth/auth_checker_settings.hpp>
#include <server/handlers/handler_config.hpp>

namespace server {
namespace handlers {
namespace auth {

AuthCheckerBasePtr CreateAuthChecker(
    const components::ComponentContext& component_context,
    const HandlerConfig& config, const AuthCheckerSettings& settings);

}  // namespace auth
}  // namespace handlers
}  // namespace server
