#include <userver/server/handlers/auth/auth_checker_settings_component.hpp>

#include <userver/components/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

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

yaml_config::Schema AuthCheckerSettings::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: >
  Component that loads auth configuration settings from a
  components::Secdist component if the latter was registered in
  components::ComponentList.
additionalProperties: false
properties: {}
)");
}

}  // namespace components

USERVER_NAMESPACE_END
