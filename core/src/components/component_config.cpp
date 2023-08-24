#include <userver/components/component_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ComponentConfig::ComponentConfig(std::string name) : name_(std::move(name)) {}

ComponentConfig::ComponentConfig(yaml_config::YamlConfig value)
    : yaml_config::YamlConfig(std::move(value)) {}

const std::string& ComponentConfig::Name() const { return name_; }

void ComponentConfig::SetName(std::string name) { name_ = std::move(name); }

ComponentConfig Parse(const yaml_config::YamlConfig& value,
                      formats::parse::To<ComponentConfig>) {
  return {value};
}

std::string GetCurrentComponentName(const ComponentConfig& config) {
  return config.Name();
}

}  // namespace components

USERVER_NAMESPACE_END
