#pragma once

#include <cstdint>

#include <json/value.h>
#include <boost/optional.hpp>

#include <json_config/variable_map.hpp>

#include "connection_config.hpp"

namespace server {
namespace net {

struct ListenerConfig {
  ConnectionConfig connection_config;
  uint16_t port = 80;
  boost::optional<uint16_t> monitor_port;
  int backlog = 1024;  // truncated to net.core.somaxconn
  size_t max_connections = 32768;
  boost::optional<size_t> shards;

  static ListenerConfig ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace net
}  // namespace server
