#include <userver/components/component_config.hpp>

namespace components {

ComponentConfig::ComponentConfig(yaml_config::YamlConfig value)
    : yaml_config::YamlConfig(std::move(value)) {}

const std::string& ComponentConfig::Name() const { return name_; }

void ComponentConfig::SetName(std::string name) { name_ = std::move(name); }

ComponentConfig Parse(const yaml_config::YamlConfig& value,
                      formats::parse::To<ComponentConfig>) {
  return ComponentConfig(value);
}

}  // namespace components
