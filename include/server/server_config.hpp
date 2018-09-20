#pragma once

#include <string>

#include <boost/optional.hpp>
#include <formats/json/value.hpp>

#include <json_config/variable_map.hpp>

#include <server/net/listener_config.hpp>

namespace server {

struct ServerConfig {
  net::ListenerConfig listener;
  net::ListenerConfig monitor_listener;
  boost::optional<std::string> logger_access;
  boost::optional<std::string> logger_access_tskv;

  json_config::VariableMapPtr config_vars_ptr;

  static ServerConfig ParseFromJson(
      const formats::json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace server
