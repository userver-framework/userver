#include <userver/components/headers_propagator_component.hpp>

#include <string>

#include <server/http/headers_propagator.hpp>
#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

HeadersPropagatorComponent::HeadersPropagatorComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      propagator_(std::make_unique<server::http::HeadersPropagator>(
          config["headers"].As<std::vector<std::string>>({}))) {}

HeadersPropagatorComponent::~HeadersPropagatorComponent() = default;

yaml_config::Schema HeadersPropagatorComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: component for propagating headers as-is
additionalProperties: false
properties:
    headers:
        type: array
        description: array of headers to propagate
        items:
            type: string
            description: header
)");
}

server::http::HeadersPropagator& HeadersPropagatorComponent::Get() {
  return *propagator_;
}

}  // namespace components

USERVER_NAMESPACE_END
