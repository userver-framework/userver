#include <server/server_config.hpp>

#include <json_config/value.hpp>

namespace server {

ServerConfig ServerConfig::ParseFromJson(
    const formats::json::Value& json, const std::string& full_path,
    const json_config::VariableMapPtr& config_vars_ptr) {
  ServerConfig config;
  config.listener = net::ListenerConfig::ParseFromJson(
      json["listener"], full_path + ".listener", config_vars_ptr);
  config.monitor_listener = net::ListenerConfig::ParseFromJson(
      json["listener-monitor"], full_path + ".listener-monitor",
      config_vars_ptr);

  config.logger_access = json_config::ParseOptionalString(
      json, "logger_access", full_path, config_vars_ptr);
  config.logger_access_tskv = json_config::ParseOptionalString(
      json, "logger_access_tskv", full_path, config_vars_ptr);
  return config;
}

}  // namespace server
