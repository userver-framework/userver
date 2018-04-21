#pragma once

#include <cstdint>

#include <json/value.h>

#include <json_config/variable_map.hpp>

#include "connection_config.hpp"

namespace server {
namespace net {

struct ListenerConfig {
  ConnectionConfig connection_config;
  uint16_t port = 80;
  uint16_t monitor_port = 1188;
  int backlog = 1024;  // truncated to net.core.somaxconn
  size_t max_connections = 32768;

  static ListenerConfig ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace net
}  // namespace server
