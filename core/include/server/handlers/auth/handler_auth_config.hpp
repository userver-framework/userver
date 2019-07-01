#pragma once

#include <string>
#include <vector>

#include <formats/yaml.hpp>

#include <yaml_config/variable_map.hpp>
#include <yaml_config/yaml_config.hpp>

namespace server {
namespace handlers {
namespace auth {

class HandlerAuthConfig final : public yaml_config::YamlConfig {
 public:
  HandlerAuthConfig(formats::yaml::Value yaml, std::string full_path,
                    yaml_config::VariableMapPtr config_vars_ptr);

  const std::vector<std::string>& GetTypes() const { return types_; };

  static HandlerAuthConfig ParseFromYaml(
      const formats::yaml::Value& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr);

 private:
  std::vector<std::string> ParseTypes();

  std::vector<std::string> types_;
};

}  // namespace auth
}  // namespace handlers
}  // namespace server
