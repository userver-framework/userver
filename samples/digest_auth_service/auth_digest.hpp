#pragma once

#include <userver/utest/using_namespace_userver.hpp>

/// [auth checker factory decl]
#include <userver/server/handlers/auth/auth_checker_factory.hpp>

namespace samples::digest_auth {

class CheckerFactory final
    : public server::handlers::auth::AuthCheckerFactoryBase {
 public:
  server::handlers::auth::AuthCheckerBasePtr operator()(
      const ::components::ComponentContext& context,
      const server::handlers::auth::HandlerAuthConfig& auth_config,
      const server::handlers::auth::AuthCheckerSettings&) const override;
};

class CheckerProxyFactory final
    : public server::handlers::auth::AuthCheckerFactoryBase {
 public:
  server::handlers::auth::AuthCheckerBasePtr operator()(
      const ::components::ComponentContext& context,
      const server::handlers::auth::HandlerAuthConfig& auth_config,
      const server::handlers::auth::AuthCheckerSettings&) const override;
};

}  // namespace samples::digest_auth
/// [auth checker factory decl]
