#include <server/server_config.hpp>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/utils/userver_info.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {

ServerConfig Parse(const yaml_config::YamlConfig& value,
                   formats::parse::To<ServerConfig>) {
  ServerConfig config;
  config.listener = value["listener"].As<net::ListenerConfig>();
  config.monitor_listener =
      value["listener-monitor"].As<std::optional<net::ListenerConfig>>();
  config.logger_access =
      value["logger_access"].As<std::optional<std::string>>();
  config.logger_access_tskv =
      value["logger_access_tskv"].As<std::optional<std::string>>();
  config.max_response_size_in_flight =
      value["max_response_size_in_flight"].As<std::optional<size_t>>();
  config.server_name =
      value["server-name"].As<std::string>(utils::GetUserverIdentifier());
  config.set_response_server_hostname =
      value["set-response-server-hostname"].As<bool>(false);

  return config;
}

}  // namespace server

USERVER_NAMESPACE_END
