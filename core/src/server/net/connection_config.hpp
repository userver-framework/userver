#pragma once

#include <memory>
#include <string>

#include <formats/yaml.hpp>

#include <server/request/request_config.hpp>
#include <yaml_config/variable_map.hpp>

namespace server {
namespace net {

struct ConnectionConfig {
  size_t in_buffer_size = 32 * 1024;
  size_t requests_queue_size_threshold = 100;
  std::chrono::seconds keepalive_timeout{10 * 60};

  std::unique_ptr<request::RequestConfig> request;

  static ConnectionConfig ParseFromYaml(
      const formats::yaml::Value& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace net
}  // namespace server
