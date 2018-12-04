#pragma once

#include <server/handlers/auth/auth_checker_settings.hpp>
#include <server/handlers/handler_config.hpp>

#include "auth_checker_base.hpp"

namespace server {
namespace handlers {
namespace auth {

AuthCheckerBasePtr CreateAuthChecker(const HandlerConfig& config,
                                     const AuthCheckerSettings& settings);

}  // namespace auth
}  // namespace handlers
}  // namespace server
