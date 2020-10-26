#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include <yaml_config/yaml_config.hpp>

namespace components {

class ComponentConfig final : public yaml_config::YamlConfig {
 public:
  ComponentConfig(yaml_config::YamlConfig value);

  const std::string& Name() const;
  void SetName(std::string name);

 private:
  std::string name_;
};

ComponentConfig Parse(const yaml_config::YamlConfig& value,
                      formats::parse::To<ComponentConfig>);

using ComponentConfigMap =
    std::unordered_map<std::string, const ComponentConfig&>;

}  // namespace components
