#include <server/handlers/auth/handler_auth_config.hpp>

#include <yaml_config/value.hpp>

namespace server {
namespace handlers {
namespace auth {

HandlerAuthConfig::HandlerAuthConfig(
    formats::yaml::Value yaml, std::string full_path,
    yaml_config::VariableMapPtr config_vars_ptr)
    : yaml_config::YamlConfig(std::move(yaml), std::move(full_path),
                              std::move(config_vars_ptr)),
      type_(ParseString("type")) {}

HandlerAuthConfig HandlerAuthConfig::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  return {yaml, full_path, config_vars_ptr};
}

}  // namespace auth
}  // namespace handlers
}  // namespace server
