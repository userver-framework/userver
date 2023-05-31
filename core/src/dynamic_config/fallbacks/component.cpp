#include <userver/dynamic_config/fallbacks/component.hpp>

#include <exception>
#include <stdexcept>
#include <string>

#include <userver/components/component.hpp>
#include <userver/dynamic_config/updates_sink/find.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

DynamicConfigFallbacks::DynamicConfigFallbacks(const ComponentConfig& config,
                                               const ComponentContext& context)
    : LoggableComponentBase(config, context),
      updates_sink_(dynamic_config::FindUpdatesSink(config, context)) {
  try {
    auto fallback_config_contents = fs::blocking::ReadFileContents(
        config["fallback-path"].As<std::string>());

    dynamic_config::DocsMap config_values;
    config_values.Parse(fallback_config_contents, false);
    auto overrides_path =
        config["overrides-path"].As<std::optional<std::string>>();
    if (overrides_path) {
      auto overrides_contents = fs::blocking::ReadFileContents(*overrides_path);
      dynamic_config::DocsMap overrides_values;
      overrides_values.Parse(overrides_contents, true);
      config_values.MergeFromOther(std::move(overrides_values));
    }

    updates_sink_.SetConfig(kName, std::move(config_values));
  } catch (const std::exception& ex) {
    throw std::runtime_error(
        std::string("Cannot load fallback dynamic config or its overrides: ") +
        ex.what());
  }
}
yaml_config::Schema DynamicConfigFallbacks::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Component that setup runtime configs based on fallbacks from file.
additionalProperties: false
properties:
    updates-sink:
        type: string
        description: components::DynamicConfigUpdatesSinkBase descendant to be used for storing fallback config
        defaultDescription: dynamic-config
    fallback-path:
        type: string
        description: a path to the fallback config file
    overrides-path:
        type: string
        description: a path to the overrides of fallback config values
)");
}

}  // namespace components

USERVER_NAMESPACE_END
