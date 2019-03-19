#include "auth_checker.hpp"

#include <server/handlers/auth/auth_checker_factory.hpp>

#include "auth_checker_allow_any.hpp"

namespace server {
namespace handlers {
namespace auth {

AuthCheckerBasePtr CreateAuthChecker(
    const components::ComponentContext& component_context,
    const HandlerConfig& config, const AuthCheckerSettings& settings) {
  if (!config.auth) return std::make_shared<AuthCheckerAllowAny>();

  const auto& factory = GetAuthCheckerFactory(config.auth->GetType());
  return factory(component_context, *config.auth, settings);
}

}  // namespace auth
}  // namespace handlers
}  // namespace server
