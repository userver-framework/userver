#include "connection_config.hpp"

#include <yaml_config/value.hpp>
#include <yaml_config/yaml_config.hpp>

namespace server::net {

ConnectionConfig ConnectionConfig::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  ConnectionConfig config;

  yaml_config::YamlConfig yaml_config(yaml, full_path, config_vars_ptr);

  auto optional_in_buffer_size =
      yaml_config.ParseOptionalUint64("in_buffer_size");
  if (optional_in_buffer_size) config.in_buffer_size = *optional_in_buffer_size;

  auto optional_queue_size_threshold =
      yaml_config.ParseOptionalUint64("requests_queue_size_threshold");
  if (optional_queue_size_threshold)
    config.requests_queue_size_threshold = *optional_queue_size_threshold;

  auto keepalive_timeout =
      yaml_config.ParseOptionalDuration("keepalive_timeout");
  if (keepalive_timeout)
    config.keepalive_timeout =
        std::chrono::duration_cast<std::chrono::seconds>(*keepalive_timeout);

  config.request = std::make_unique<request::RequestConfig>(
      request::RequestConfig::ParseFromYaml(
          yaml["request"], full_path + ".request", config_vars_ptr));
  return config;
}

}  // namespace server::net
