#pragma once

#include <memory>
#include <string>

#include <json/value.h>

#include <server/request/request_config.hpp>
#include <yandex/taxi/userver/json_config/variable_map.hpp>

namespace server {
namespace net {

struct ConnectionConfig {
  size_t in_buffer_size = 32 * 1024;
  size_t requests_queue_size_threshold = 100;
  std::unique_ptr<request::RequestConfig> request;

  static ConnectionConfig ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace net
}  // namespace server
