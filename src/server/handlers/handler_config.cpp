#include <server/handlers/handler_config.hpp>

#include <yaml_config/value.hpp>

namespace server {
namespace handlers {

HandlerConfig HandlerConfig::ParseFromYaml(
    const formats::yaml::Node& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  HandlerConfig config;
  config.path =
      yaml_config::ParseString(yaml, "path", full_path, config_vars_ptr);
  config.task_processor = yaml_config::ParseString(yaml, "task_processor",
                                                   full_path, config_vars_ptr);
  config.method = yaml_config::ParseOptionalString(yaml, "method", full_path,
                                                   config_vars_ptr);
  config.max_url_size = yaml_config::ParseOptionalUint64(
      yaml, "max_url_size", full_path, config_vars_ptr);
  config.max_request_size = yaml_config::ParseOptionalUint64(
      yaml, "max_request_size", full_path, config_vars_ptr);
  config.max_headers_size = yaml_config::ParseOptionalUint64(
      yaml, "max_headers_size", full_path, config_vars_ptr);
  config.parse_args_from_body = yaml_config::ParseOptionalBool(
      yaml, "parse_args_from_body", full_path, config_vars_ptr);

  return config;
}

}  // namespace handlers
}  // namespace server
