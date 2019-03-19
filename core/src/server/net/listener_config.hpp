#pragma once

#include <cstdint>

#include <boost/optional.hpp>
#include <formats/yaml.hpp>

#include <yaml_config/variable_map.hpp>

#include "connection_config.hpp"

namespace server {
namespace net {

struct ListenerConfig {
  ConnectionConfig connection_config;
  std::string unix_socket_path;
  uint16_t port = 0;
  int backlog = 1024;  // truncated to net.core.somaxconn
  size_t max_connections = 32768;
  boost::optional<size_t> shards;
  std::string task_processor;

  static ListenerConfig ParseFromYaml(
      const formats::yaml::Value& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace net
}  // namespace server
