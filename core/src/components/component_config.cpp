#include <components/component_config.hpp>

namespace components {

ComponentConfig::ComponentConfig(formats::yaml::Value yaml,
                                 std::string full_path,
                                 yaml_config::VariableMapPtr config_vars_ptr)
    : yaml_config::YamlConfig(std::move(yaml), std::move(full_path),
                              std::move(config_vars_ptr)) {}

const std::string& ComponentConfig::Name() const { return name_; }

void ComponentConfig::SetName(const std::string& name) { name_ = name; }

ComponentConfig ComponentConfig::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  return {yaml, full_path, config_vars_ptr};
}

}  // namespace components
