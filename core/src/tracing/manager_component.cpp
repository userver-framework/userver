#include <userver/tracing/manager_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/tracing/span_builder.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {
const TracingManagerBase& GetTracingManagerFromConfig(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  if (config.HasMember("component-name")) {
    auto tracing_manager_name = config["component-name"].As<std::string>();
    if (!tracing_manager_name.empty()) {
      return context.FindComponent<TracingManagerBase>(tracing_manager_name);
    }
  }
  return kDefaultTracingManager;
}
}  // namespace

TracingManagerComponentBase::TracingManagerComponentBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context) {}

DefaultTracingManagerLocator::DefaultTracingManagerLocator(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      tracing_manager_(GetTracingManagerFromConfig(config, context)) {}

const TracingManagerBase& DefaultTracingManagerLocator::GetTracingManager()
    const {
  return tracing_manager_;
}

yaml_config::Schema DefaultTracingManagerLocator::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: component for finding actual tracing manager
additionalProperties: false
properties:
    component-name:
        type: string
        description: tracing manager component's name
)");
}

}  // namespace tracing

USERVER_NAMESPACE_END
