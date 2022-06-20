#include <userver/components/component_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ComponentConfig::ComponentConfig(std::string name) : name_(std::move(name)) {}

ComponentConfig::ComponentConfig(yaml_config::YamlConfig value)
    : yaml_config::YamlConfig(std::move(value)) {}

const std::string& ComponentConfig::Name() const { return name_; }

void ComponentConfig::SetName(std::string name) {
  if (name == "taxi-configs-client") name = "dynamic-config-client";
  if (name == "taxi-config-fallbacks") name = "dynamic-config-fallbacks";
  if (name == "taxi-config") name = "dynamic-config";
  if (name == "taxi-config-client-updater")
    name = "dynamic-config-client-updater";

  name_ = std::move(name);
}

ComponentConfig Parse(const yaml_config::YamlConfig& value,
                      formats::parse::To<ComponentConfig>) {
  return {value};
}

std::string GetCurrentComponentName(const ComponentConfig& config) {
  return config.Name();
}

}  // namespace components

USERVER_NAMESPACE_END
