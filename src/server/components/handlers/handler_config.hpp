#pragma once

#include <string>

#include <json/value.h>

#include <json_config/variable_map.hpp>

namespace components {
namespace handlers {

struct HandlerConfig {
  std::string path;
  std::string task_processor;

  static HandlerConfig ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace handlers
}  // namespace components
