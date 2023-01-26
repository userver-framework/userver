#pragma once

#include <cstdint>
#include <optional>

#include <userver/server/request/request_config.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include "connection_config.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::net {

struct ListenerConfig {
  ConnectionConfig connection_config;
  request::HttpRequestConfig handler_defaults;
  std::string unix_socket_path;
  uint16_t port = 0;
  int backlog = 1024;  // truncated to net.core.somaxconn
  size_t max_connections = 32768;
  std::optional<size_t> shards;
  std::string task_processor;
};

ListenerConfig Parse(const yaml_config::YamlConfig& value,
                     formats::parse::To<ListenerConfig>);

}  // namespace server::net

USERVER_NAMESPACE_END
