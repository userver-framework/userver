#pragma once

#include <server/handlers/auth/auth_checker_base.hpp>
#include <server/handlers/auth/auth_checker_factory.hpp>
#include <server/handlers/auth/auth_checker_settings.hpp>
#include <server/handlers/auth/handler_auth_config.hpp>

namespace server {
namespace handlers {
namespace auth {

class AuthCheckerApiKeyFactory : public AuthCheckerFactoryBase {
 public:
  static constexpr const char* kAuthType = "apikey";

  AuthCheckerBasePtr operator()(
      const ::components::ComponentContext&,
      const server::handlers::auth::HandlerAuthConfig&,
      const server::handlers::auth::AuthCheckerSettings&) const override;
};

}  // namespace auth
}  // namespace handlers
}  // namespace server
