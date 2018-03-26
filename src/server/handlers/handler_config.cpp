#include "handler_config.hpp"

#include <json_config/value.hpp>

namespace server {
namespace handlers {

HandlerConfig HandlerConfig::ParseFromJson(
    const Json::Value& json, const std::string& full_path,
    const json_config::VariableMapPtr& config_vars_ptr) {
  HandlerConfig config;
  config.path =
      json_config::ParseString(json, "path", full_path, config_vars_ptr);
  config.task_processor = json_config::ParseString(json, "task_processor",
                                                   full_path, config_vars_ptr);
  return config;
}

}  // namespace handlers
}  // namespace server
