#pragma once

#include <optional>
#include <string>

#include <userver/yaml_config/yaml_config.hpp>

#include <server/net/listener_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {

struct ServerConfig {
  net::ListenerConfig listener;
  std::optional<net::ListenerConfig> monitor_listener;
  std::optional<std::string> logger_access;
  std::optional<std::string> logger_access_tskv;
  std::optional<size_t> max_response_size_in_flight;
  std::string server_name;
  bool set_response_server_hostname{false};
};

ServerConfig Parse(const yaml_config::YamlConfig& value,
                   formats::parse::To<ServerConfig>);

}  // namespace server

USERVER_NAMESPACE_END
