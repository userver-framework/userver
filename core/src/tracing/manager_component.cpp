#include <userver/tracing/manager_component.hpp>

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/tracing/span_builder.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {

using FlagsFormat = utils::Flags<tracing::Format>;

}  // namespace

const TracingManagerBase& GetTracingManagerFromConfig(
    const GenericTracingManager& default_manager,
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  if (config.HasMember("component-name")) {
    auto tracing_manager_name = config["component-name"].As<std::string>();
    if (!tracing_manager_name.empty()) {
      return context.FindComponent<TracingManagerBase>(tracing_manager_name);
    }
  }
  return default_manager;
}

FlagsFormat Parse(const yaml_config::YamlConfig& value,
                  formats::parse::To<FlagsFormat>) {
  utils::Flags<tracing::Format> format = tracing::Format{};

  if (!value.IsArray()) {
    format |= tracing::FormatFromString(value.As<std::string>("taxi"));
  } else {
    for (const auto& f : value) {
      format |= tracing::FormatFromString(f.As<std::string>());
    }
  }

  return format;
}

TracingManagerComponentBase::TracingManagerComponentBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context) {}

DefaultTracingManagerLocator::DefaultTracingManagerLocator(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context),
      default_manager_(config["incoming-format"].As<FlagsFormat>(),
                       config["new-requests-format"].As<FlagsFormat>()),
      tracing_manager_(
          GetTracingManagerFromConfig(default_manager_, config, context)) {}

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
    incoming-format:
        type: array
        description: Incoming tracing data formats
        items: &format_items
            type: string
            description: tracing formats
            enum:
              - b3-alternative
              - yandex
              - taxi
              - opentelemetry
    new-requests-format:
        type: array
        description: Send tracing data in those formats
        items: *format_items
)");
}

}  // namespace tracing

USERVER_NAMESPACE_END
