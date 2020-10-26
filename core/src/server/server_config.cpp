#include <server/server_config.hpp>

namespace server {

ServerConfig Parse(const yaml_config::YamlConfig& value,
                   formats::parse::To<ServerConfig>) {
  ServerConfig config;
  config.listener = value["listener"].As<net::ListenerConfig>();
  config.monitor_listener = value["listener-monitor"].As<net::ListenerConfig>();
  config.logger_access =
      value["logger_access"].As<std::optional<std::string>>();
  config.logger_access_tskv =
      value["logger_access_tskv"].As<std::optional<std::string>>();
  config.max_response_size_in_flight =
      value["max_response_size_in_flight"].As<std::optional<size_t>>();
  return config;
}

}  // namespace server
