#pragma once

#include <cstdint>
#include <optional>

#include <yaml_config/yaml_config.hpp>

#include "connection_config.hpp"

namespace server {
namespace net {

struct ListenerConfig {
  ConnectionConfig connection_config;
  std::string unix_socket_path;
  uint16_t port = 0;
  int backlog = 1024;  // truncated to net.core.somaxconn
  size_t max_connections = 32768;
  std::optional<size_t> shards;
  std::string task_processor;
};

ListenerConfig Parse(const yaml_config::YamlConfig& value,
                     formats::parse::To<ListenerConfig>);

}  // namespace net
}  // namespace server
