#pragma once

#include <string>

#include <formats/yaml.hpp>

#include <yaml_config/variable_map.hpp>
#include <yaml_config/yaml_config.hpp>

namespace server {
namespace handlers {
namespace auth {

enum class AuthType { kApiKey };

std::string ToString(AuthType auth_type);

class HandlerAuthConfig : public yaml_config::YamlConfig {
 public:
  HandlerAuthConfig(formats::yaml::Node yaml, std::string full_path,
                    yaml_config::VariableMapPtr config_vars_ptr);

  AuthType GetType() const { return type_; };

  static HandlerAuthConfig ParseFromYaml(
      const formats::yaml::Node& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr);

 private:
  AuthType type_;
};

}  // namespace auth
}  // namespace handlers
}  // namespace server
