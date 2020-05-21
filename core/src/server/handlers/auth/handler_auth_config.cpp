#include <server/handlers/auth/handler_auth_config.hpp>

#include <stdexcept>

#include <yaml_config/value.hpp>

namespace server::handlers::auth {

HandlerAuthConfig::HandlerAuthConfig(
    formats::yaml::Value yaml, std::string full_path,
    yaml_config::VariableMapPtr config_vars_ptr)
    : yaml_config::YamlConfig(std::move(yaml), std::move(full_path),
                              std::move(config_vars_ptr)),
      types_(ParseTypes()) {
  if (types_.empty())
    throw std::runtime_error("types list is empty in handler auth config");
}

HandlerAuthConfig HandlerAuthConfig::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  return {yaml, full_path, config_vars_ptr};
}

std::vector<std::string> HandlerAuthConfig::ParseTypes() {
  static const std::string kTypes = "types";
  static const std::string kType = "type";

  auto types_opt = Parse<std::optional<std::vector<std::string>>>(kTypes);
  auto type_opt = Parse<std::optional<std::string>>(kType);

  if (types_opt && type_opt) {
    throw std::runtime_error("invalid handler auth config: both fields '" +
                             kTypes + "' and '" + kType +
                             "' are set, exactly one is expected");
  }

  if (types_opt) return std::move(*types_opt);
  if (type_opt) return {std::move(*type_opt)};
  throw std::runtime_error("invalid handler auth config: none of fields '" +
                           kTypes + "' and '" + kType + "' was found");
}

}  // namespace server::handlers::auth
