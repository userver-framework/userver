#include <server/handlers/auth/auth_checker_settings_component.hpp>

namespace components {

AuthCheckerSettings::AuthCheckerSettings(
    const ComponentConfig& component_config,
    const ComponentContext& component_context)
    : LoggableComponentBase(component_config, component_context),
      secdist_component_(component_context.FindComponent<Secdist>()),
      settings_(secdist_component_.Get()
                    .Get<server::handlers::auth::AuthCheckerSettings>()) {}

}  // namespace components
