#pragma once

#include <string>

#include <json/value.h>
#include <boost/optional.hpp>

#include <yandex/taxi/userver/json_config/variable_map.hpp>

#include <server/net/listener_config.hpp>

namespace server {

struct ServerConfig {
  net::ListenerConfig listener;
  boost::optional<std::string> logger_access;
  boost::optional<std::string> logger_access_tskv;
  std::string task_processor;

  json_config::VariableMapPtr config_vars_ptr;

  static ServerConfig ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace server
