#include <userver/server/handlers/auth/auth_checker_settings_component.hpp>

namespace components {

namespace {

server::handlers::auth::AuthCheckerSettings GetSettings(Secdist* secdist) {
  if (secdist) {
    return secdist->Get().Get<server::handlers::auth::AuthCheckerSettings>();
  }

  return server::handlers::auth::AuthCheckerSettings{formats::json::Value{}};
}

}  // namespace

AuthCheckerSettings::AuthCheckerSettings(
    const ComponentConfig& component_config,
    const ComponentContext& component_context)
    : LoggableComponentBase(component_config, component_context),
      settings_(
          GetSettings(component_context.FindComponentOptional<Secdist>())) {}

}  // namespace components
