#pragma once

#include <optional>
#include <string>

#include <formats/yaml.hpp>

#include <yaml_config/variable_map.hpp>

#include <server/net/listener_config.hpp>

namespace server {

struct ServerConfig {
  net::ListenerConfig listener;
  net::ListenerConfig monitor_listener;
  std::optional<std::string> logger_access;
  std::optional<std::string> logger_access_tskv;
  std::optional<size_t> max_response_size_in_flight;

  yaml_config::VariableMapPtr config_vars_ptr;

  static ServerConfig ParseFromYaml(
      const formats::yaml::Value& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace server
