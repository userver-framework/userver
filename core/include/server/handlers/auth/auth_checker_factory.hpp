#pragma once

#include <components/component_context.hpp>
#include <server/handlers/auth/auth_checker_base.hpp>
#include <server/handlers/auth/auth_checker_settings.hpp>
#include <server/handlers/handler_config.hpp>

namespace server {
namespace handlers {
namespace auth {

class AuthCheckerFactoryBase {
 public:
  virtual ~AuthCheckerFactoryBase() = default;

  virtual AuthCheckerBasePtr operator()(const components::ComponentContext&,
                                        const HandlerAuthConfig&,
                                        const AuthCheckerSettings&) const = 0;
};

void RegisterAuthCheckerFactory(
    std::string auth_type, std::unique_ptr<AuthCheckerFactoryBase>&& factory);
const AuthCheckerFactoryBase& GetAuthCheckerFactory(
    const std::string& auth_type);

}  // namespace auth
}  // namespace handlers
}  // namespace server
