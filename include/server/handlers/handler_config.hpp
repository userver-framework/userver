#pragma once

#include <string>

#include <boost/optional.hpp>
#include <formats/json/value.hpp>

#include <json_config/variable_map.hpp>

namespace server {
namespace handlers {

struct HandlerConfig {
  std::string path;
  std::string task_processor;
  boost::optional<size_t> max_url_size;
  boost::optional<size_t> max_request_size;
  boost::optional<size_t> max_headers_size;
  boost::optional<bool> parse_args_from_body;

  static HandlerConfig ParseFromJson(
      const formats::json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace handlers
}  // namespace server
