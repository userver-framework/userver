#include <server/handlers/auth/auth_checker_apikey_factory.hpp>

#include "auth_checker_apikey.hpp"

namespace server {
namespace handlers {
namespace auth {
namespace {

bool RegisterAuthChecker() {
  RegisterAuthCheckerFactory(AuthCheckerApiKeyFactory::kAuthType,
                             std::make_unique<AuthCheckerApiKeyFactory>());
  return true;
}

}  // namespace

AuthCheckerBasePtr AuthCheckerApiKeyFactory::operator()(
    const ::components::ComponentContext&,
    const server::handlers::auth::HandlerAuthConfig& config,
    const server::handlers::auth::AuthCheckerSettings& settings) const {
  return std::make_shared<AuthCheckerApiKey>(config, settings);
}

static const bool kAuthCheckerFactoryRegistered{RegisterAuthChecker()};

// Should be touched from main program.
// Otherwise linker will discard auth-checker-apikey registration.
int auth_checker_apikey_module_activation;

}  // namespace auth
}  // namespace handlers
}  // namespace server
