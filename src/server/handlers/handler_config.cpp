#include <server/handlers/handler_config.hpp>

#include <json_config/value.hpp>

namespace server {
namespace handlers {

HandlerConfig HandlerConfig::ParseFromJson(
    const formats::json::Value& json, const std::string& full_path,
    const json_config::VariableMapPtr& config_vars_ptr) {
  HandlerConfig config;
  config.path =
      json_config::ParseString(json, "path", full_path, config_vars_ptr);
  config.task_processor = json_config::ParseString(json, "task_processor",
                                                   full_path, config_vars_ptr);
  config.max_url_size = json_config::ParseOptionalUint64(
      json, "max_url_size", full_path, config_vars_ptr);
  config.max_request_size = json_config::ParseOptionalUint64(
      json, "max_request_size", full_path, config_vars_ptr);
  config.max_headers_size = json_config::ParseOptionalUint64(
      json, "max_headers_size", full_path, config_vars_ptr);
  config.parse_args_from_body = json_config::ParseOptionalBool(
      json, "parse_args_from_body", full_path, config_vars_ptr);

  return config;
}

}  // namespace handlers
}  // namespace server
