#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

#include <formats/yaml.hpp>

#include <yaml_config/variable_map.hpp>
#include <yaml_config/yaml_config.hpp>

namespace components {

class ComponentConfig : public yaml_config::YamlConfig {
 public:
  ComponentConfig(formats::yaml::Value yaml, std::string full_path,
                  yaml_config::VariableMapPtr config_vars_ptr);

  const std::string& Name() const;
  void SetName(const std::string& name);

  static ComponentConfig ParseFromYaml(
      const formats::yaml::Value& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr);

 private:
  std::string name_;
};

using ComponentConfigMap =
    std::unordered_map<std::string, const ComponentConfig&>;

}  // namespace components
