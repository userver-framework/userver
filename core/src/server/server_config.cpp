#include <server/server_config.hpp>

#include <yaml_config/value.hpp>

namespace server {

ServerConfig ServerConfig::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  ServerConfig config;
  config.listener = net::ListenerConfig::ParseFromYaml(
      yaml["listener"], full_path + ".listener", config_vars_ptr);
  config.monitor_listener = net::ListenerConfig::ParseFromYaml(
      yaml["listener-monitor"], full_path + ".listener-monitor",
      config_vars_ptr);

  config.logger_access = yaml_config::ParseOptionalString(
      yaml, "logger_access", full_path, config_vars_ptr);
  config.logger_access_tskv = yaml_config::ParseOptionalString(
      yaml, "logger_access_tskv", full_path, config_vars_ptr);

  config.max_response_size_in_flight = yaml_config::ParseOptionalUint64(
      yaml, "max_response_size_in_flight", full_path, config_vars_ptr);
  return config;
}

}  // namespace server
