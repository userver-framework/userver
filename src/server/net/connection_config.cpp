#include "connection_config.hpp"

#include <yandex/taxi/userver/json_config/value.hpp>

namespace server {
namespace net {

ConnectionConfig ConnectionConfig::ParseFromJson(
    const Json::Value& json, const std::string& full_path,
    const json_config::VariableMapPtr& config_vars_ptr) {
  ConnectionConfig config;
  auto optional_in_buffer_size = json_config::ParseOptionalUint64(
      json, "in_buffer_size", full_path, config_vars_ptr);
  if (optional_in_buffer_size) config.in_buffer_size = *optional_in_buffer_size;
  auto optional_queue_size_threshold = json_config::ParseOptionalUint64(
      json, "requests_queue_size_threshold", full_path, config_vars_ptr);
  if (optional_queue_size_threshold)
    config.requests_queue_size_threshold = *optional_queue_size_threshold;
  config.request = std::make_unique<request::RequestConfig>(
      request::RequestConfig::ParseFromJson(
          json["request"], full_path + ".request", config_vars_ptr));
  return config;
}

}  // namespace net
}  // namespace server
