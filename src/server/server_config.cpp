#include <yandex/taxi/userver/server/server_config.hpp>

#include <yandex/taxi/userver/json_config/value.hpp>

namespace server {

ServerConfig ServerConfig::ParseFromJson(
    const Json::Value& json, const std::string& full_path,
    const json_config::VariableMapPtr& config_vars_ptr) {
  ServerConfig config;
  config.listener = net::ListenerConfig::ParseFromJson(
      json["listener"], full_path + ".listener", config_vars_ptr);
  config.logger_access = json_config::ParseOptionalString(
      json, "logger_access", full_path, config_vars_ptr);
  config.logger_access_tskv = json_config::ParseOptionalString(
      json, "logger_access_tskv", full_path, config_vars_ptr);
  config.task_processor = json_config::ParseString(json, "task_processor",
                                                   full_path, config_vars_ptr);
  return config;
}

}  // namespace server
