#pragma once

#include <userver/server/handlers/auth/auth_checker_factory.hpp>

namespace real_medium::auth {

class CheckerFactory final
    : public userver::server::handlers::auth::AuthCheckerFactoryBase {
 public:
  userver::server::handlers::auth::AuthCheckerBasePtr operator()(
      const userver::components::ComponentContext& context,
      const userver::server::handlers::auth::HandlerAuthConfig& auth_config,
      const userver::server::handlers::auth::AuthCheckerSettings&)
      const override;
};

}  // namespace real_medium::auth