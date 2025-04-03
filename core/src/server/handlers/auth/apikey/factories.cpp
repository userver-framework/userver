#include <server/handlers/auth/apikey/factories.hpp>

#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/handlers/auth/auth_checker_settings.hpp>
#include <userver/server/handlers/auth/auth_checker_settings_component.hpp>
#include <userver/server/handlers/auth/handler_auth_config.hpp>

#include "auth_checker_apikey.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::apikey {
namespace {

class AuthCheckerApiKeyFactory final : public AuthCheckerFactoryBase {
public:
    static constexpr std::string_view kAuthType = "apikey";

    explicit AuthCheckerApiKeyFactory(const components::ComponentContext& context)
        : settings_(context.FindComponent<components::AuthCheckerSettings>().Get()) {}

    AuthCheckerBasePtr MakeAuthChecker(const HandlerAuthConfig& config) const override {
        return std::make_shared<AuthCheckerApiKey>(config, settings_);
    }

private:
    const AuthCheckerSettings& settings_;
};

}  // namespace

// Should be touched from main program.
// Otherwise, linker will discard auth-checker-apikey registration.
int auth_checker_apikey_module_activation = (RegisterAuthCheckerFactory<AuthCheckerApiKeyFactory>(), 0);

}  // namespace server::handlers::auth::apikey

USERVER_NAMESPACE_END
