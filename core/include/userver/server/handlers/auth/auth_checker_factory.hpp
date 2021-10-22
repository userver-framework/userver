#pragma once

/// @file userver/server/handlers/auth/auth_checker_factory.hpp
/// @brief Authorization factory registration and base classes.

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/handlers/auth/auth_checker_settings.hpp>
#include <userver/server/handlers/handler_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

/// Base class for all the authorization factory checkers
class AuthCheckerFactoryBase {
 public:
  virtual ~AuthCheckerFactoryBase() = default;

  virtual AuthCheckerBasePtr operator()(const components::ComponentContext&,
                                        const HandlerAuthConfig&,
                                        const AuthCheckerSettings&) const = 0;
};

/// Function to call from main() to register an authorization checker
void RegisterAuthCheckerFactory(
    std::string auth_type, std::unique_ptr<AuthCheckerFactoryBase>&& factory);

/// Function that returns an authorization checker factory
const AuthCheckerFactoryBase& GetAuthCheckerFactory(
    const std::string& auth_type);

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
