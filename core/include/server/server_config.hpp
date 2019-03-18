#pragma once

#include <string>

#include <boost/optional.hpp>
#include <formats/yaml.hpp>

#include <yaml_config/variable_map.hpp>

#include <server/net/listener_config.hpp>

namespace server {

struct ServerConfig {
  net::ListenerConfig listener;
  net::ListenerConfig monitor_listener;
  boost::optional<std::string> logger_access;
  boost::optional<std::string> logger_access_tskv;

  yaml_config::VariableMapPtr config_vars_ptr;

  static ServerConfig ParseFromYaml(
      const formats::yaml::Value& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace server
