#include "connection_config.hpp"

#include <yaml_config/yaml_config.hpp>

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
  config.request = value["request"].As<request::RequestConfig>();

  return config;
}

}  // namespace server::net
