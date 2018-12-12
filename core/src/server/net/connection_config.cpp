#include "connection_config.hpp"

#include <yaml_config/value.hpp>

namespace server {
namespace net {

ConnectionConfig ConnectionConfig::ParseFromYaml(
    const formats::yaml::Node& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  ConnectionConfig config;
  auto optional_in_buffer_size = yaml_config::ParseOptionalUint64(
      yaml, "in_buffer_size", full_path, config_vars_ptr);
  if (optional_in_buffer_size) config.in_buffer_size = *optional_in_buffer_size;
  auto optional_queue_size_threshold = yaml_config::ParseOptionalUint64(
      yaml, "requests_queue_size_threshold", full_path, config_vars_ptr);
  if (optional_queue_size_threshold)
    config.requests_queue_size_threshold = *optional_queue_size_threshold;
  config.request = std::make_unique<request::RequestConfig>(
      request::RequestConfig::ParseFromYaml(
          yaml["request"], full_path + ".request", config_vars_ptr));
  return config;
}

}  // namespace net
}  // namespace server
