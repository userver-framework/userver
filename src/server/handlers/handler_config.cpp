#include <server/handlers/handler_config.hpp>

#include <yaml_config/value.hpp>

namespace server {
namespace handlers {

HandlerConfig HandlerConfig::ParseFromYaml(
    const formats::yaml::Node& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  HandlerConfig config;
  yaml_config::ParseInto(config.path, yaml, "path", full_path, config_vars_ptr);
  yaml_config::ParseInto(config.task_processor, yaml, "task_processor",
                         full_path, config_vars_ptr);
  yaml_config::ParseInto(config.method, yaml, "method", full_path,
                         config_vars_ptr);
  yaml_config::ParseInto(config.max_url_size, yaml, "max_url_size", full_path,
                         config_vars_ptr);
  yaml_config::ParseInto(config.max_request_size, yaml, "max_request_size",
                         full_path, config_vars_ptr);
  yaml_config::ParseInto(config.max_headers_size, yaml, "max_headers_size",
                         full_path, config_vars_ptr);
  yaml_config::ParseInto(config.parse_args_from_body, yaml,
                         "parse_args_from_body", full_path, config_vars_ptr);
  yaml_config::ParseInto(config.auth, yaml, "auth", full_path, config_vars_ptr);

  return config;
}

}  // namespace handlers
}  // namespace server
