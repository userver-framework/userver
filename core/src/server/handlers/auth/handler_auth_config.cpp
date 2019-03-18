#include <server/handlers/auth/handler_auth_config.hpp>

#include <yaml_config/value.hpp>

namespace server {
namespace handlers {
namespace auth {
namespace {

const std::string kApiKey = "apikey";

AuthType ParseAuthType(const std::string& str) {
  if (str == kApiKey) return AuthType::kApiKey;
  throw std::runtime_error("can't parse AuthType from '" + str + '\'');
}

}  // namespace

std::string ToString(AuthType auth_type) {
  switch (auth_type) {
    case AuthType::kApiKey:
      return kApiKey;
  }
  return "<unknown>(" + std::to_string(static_cast<int>(auth_type)) + ')';
}

HandlerAuthConfig::HandlerAuthConfig(
    formats::yaml::Value yaml, std::string full_path,
    yaml_config::VariableMapPtr config_vars_ptr)
    : yaml_config::YamlConfig(std::move(yaml), std::move(full_path),
                              std::move(config_vars_ptr)),
      type_(ParseAuthType(ParseString("type"))) {}

HandlerAuthConfig HandlerAuthConfig::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  return {yaml, full_path, config_vars_ptr};
}

}  // namespace auth
}  // namespace handlers
}  // namespace server
