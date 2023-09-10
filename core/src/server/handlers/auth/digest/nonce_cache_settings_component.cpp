#include <userver/server/handlers/auth/digest/nonce_cache_settings_component.hpp>

#include <userver/components/component.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest {

NonceCacheSettingsComponent::NonceCacheSettingsComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : AuthCheckerSettingsComponent(config, context) {
  settings_.ways = config["ways"].As<std::size_t>();
  settings_.way_size = config["size"].As<std::size_t>();
}

NonceCacheSettingsComponent::~NonceCacheSettingsComponent() = default;

const NonceCacheSettings& NonceCacheSettingsComponent::GetSettings() const {
  return settings_;
}

yaml_config::Schema NonceCacheSettingsComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<AuthCheckerSettingsComponent>(R"(
type: object
description: class for nonce cache settings
additionalProperties: false
properties:
    ways:
        type: integer
        description: number of ways for associative cache
    size:
        type: integer
        description: max amount of items to store in cache
)");
}

}  // namespace server::handlers::auth::digest

USERVER_NAMESPACE_END
