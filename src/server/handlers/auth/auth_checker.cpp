#include "auth_checker.hpp"

#include "auth_checker_allow_any.hpp"
#include "auth_checker_apikey.hpp"

namespace server {
namespace handlers {
namespace auth {

AuthCheckerBasePtr CreateAuthChecker(const HandlerConfig& config,
                                     const AuthCheckerSettings& settings) {
  if (!config.auth) return std::make_unique<AuthCheckerAllowAny>();

  switch (config.auth->GetType()) {
    case AuthType::kApiKey:
      return std::make_unique<AuthCheckerApiKey>(*config.auth, settings);
  }
  throw std::runtime_error("can't create AuthChecker for auth type " +
                           ToString(config.auth->GetType()));
}

}  // namespace auth
}  // namespace handlers
}  // namespace server
