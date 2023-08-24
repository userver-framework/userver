#pragma once

#include <userver/utest/using_namespace_userver.hpp>  // only for samples

/// [auth checker factory decl]
#include <userver/server/handlers/auth/auth_checker_factory.hpp>

namespace samples::pg {

class CheckerFactory final
    : public server::handlers::auth::AuthCheckerFactoryBase {
 public:
  server::handlers::auth::AuthCheckerBasePtr operator()(
      const ::components::ComponentContext& context,
      const server::handlers::auth::HandlerAuthConfig& auth_config,
      const server::handlers::auth::AuthCheckerSettings&) const override;
};

}  // namespace samples::pg
/// [auth checker factory decl]
