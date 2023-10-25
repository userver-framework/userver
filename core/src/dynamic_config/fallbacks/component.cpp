#include <userver/dynamic_config/fallbacks/component.hpp>

#include <exception>
#include <stdexcept>
#include <string>

#include <userver/components/component.hpp>
#include <userver/dynamic_config/updates_sink/find.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <utils/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

// TODO deduplicate with DynamicConfigClientUpdater
dynamic_config::DocsMap ParseFallbacksAndOverrides(
    const ComponentConfig& component_config) {
  dynamic_config::DocsMap config_values;
  {
    const auto fallback_path =
        component_config["fallback-path"].As<std::optional<std::string>>();
    if (fallback_path) {
      try {
        const auto fallback_config_contents =
            fs::blocking::ReadFileContents(*fallback_path);
        config_values.Parse(fallback_config_contents, false);
      } catch (const std::exception& ex) {
        throw std::runtime_error(fmt::format(
            "Failed to load fallback dynamic config: {}", ex.what()));
      }
    } else {
      try {
        config_values = dynamic_config::impl::MakeDefaultDocsMap();
      } catch (const std::exception& ex) {
        throw std::runtime_error(fmt::format(
            "Failed to create dynamic config defaults: {}", ex.what()));
      }
    }
  }
  {
    auto default_overrides =
        component_config["defaults"].As<std::optional<formats::json::Value>>();
    if (default_overrides) {
      dynamic_config::DocsMap overrides;
      for (const auto& [key, value] : Items(*default_overrides)) {
        overrides.Set(key, value);
      }
      config_values.MergeOrAssign(std::move(overrides));
    }
  }
  return config_values;
}

}  // namespace

DynamicConfigFallbacks::DynamicConfigFallbacks(const ComponentConfig& config,
                                               const ComponentContext& context)
    : LoggableComponentBase(config, context),
      updates_sink_(dynamic_config::FindUpdatesSink(config, context)) {
  try {
    defaults_ = ParseFallbacksAndOverrides(config);
    updates_sink_.SetConfig(kName, defaults_);
  } catch (const std::exception& ex) {
    throw std::runtime_error(
        std::string("Cannot load fallback dynamic config or its overrides: ") +
        ex.what());
  }
}

const dynamic_config::DocsMap& DynamicConfigFallbacks::GetDefaults(
    utils::InternalTag) const {
  return defaults_;
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
        defaultDescription: defaults are taken from dynamic_config::Key definitions
    defaults:
        type: object
        description: optional values for configs that override the defaults specified in dynamic_config::Key definitions
        properties: {}
        additionalProperties: true
)");
}

}  // namespace components

USERVER_NAMESPACE_END
