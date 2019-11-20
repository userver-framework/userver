#include <server/handlers/auth/auth_checker_base.hpp>
#include <server/handlers/auth/auth_checker_factory.hpp>
#include <server/handlers/auth/auth_checker_settings.hpp>
#include <server/handlers/auth/handler_auth_config.hpp>

#include "auth_checker_apikey.hpp"

namespace server::handlers::auth::apikey {
namespace {

class AuthCheckerApiKeyFactory final : public AuthCheckerFactoryBase {
 public:
  static constexpr const char* kAuthType = "apikey";

  AuthCheckerBasePtr operator()(
      const ::components::ComponentContext&, const HandlerAuthConfig& config,
      const AuthCheckerSettings& settings) const override {
    return std::make_shared<AuthCheckerApiKey>(config, settings);
  }
};

bool RegisterAuthChecker() {
  RegisterAuthCheckerFactory(AuthCheckerApiKeyFactory::kAuthType,
                             std::make_unique<AuthCheckerApiKeyFactory>());
  return true;
}

}  // namespace

static const bool kAuthCheckerFactoryRegistered{RegisterAuthChecker()};

// Should be touched from main program.
// Otherwise linker will discard auth-checker-apikey registration.
int auth_checker_apikey_module_activation;

}  // namespace server::handlers::auth::apikey
