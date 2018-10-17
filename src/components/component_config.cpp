#include <components/component_config.hpp>

namespace components {

ComponentConfig::ComponentConfig(formats::yaml::Node yaml,
                                 std::string full_path,
                                 yaml_config::VariableMapPtr config_vars_ptr)
    : yaml_config::YamlConfig(std::move(yaml), std::move(full_path),
                              std::move(config_vars_ptr)) {
  name_ = ParseString("name");
}

const std::string& ComponentConfig::Name() const { return name_; }

ComponentConfig ComponentConfig::ParseFromYaml(
    const formats::yaml::Node& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  return {yaml, full_path, config_vars_ptr};
}

}  // namespace components
