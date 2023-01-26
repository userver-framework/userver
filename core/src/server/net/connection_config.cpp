#include <server/net/connection_config.hpp>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

ConnectionConfig Parse(const yaml_config::YamlConfig& value,
                       formats::parse::To<ConnectionConfig>) {
  ConnectionConfig config;

  config.in_buffer_size =
      value["in_buffer_size"].As<size_t>(config.in_buffer_size);
  config.requests_queue_size_threshold =
      value["requests_queue_size_threshold"].As<size_t>(
          config.requests_queue_size_threshold);
  config.keepalive_timeout =
      value["keepalive_timeout"].As<std::chrono::seconds>(
          config.keepalive_timeout);

  return config;
}

}  // namespace server::net

USERVER_NAMESPACE_END
