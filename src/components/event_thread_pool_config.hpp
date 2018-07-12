#pragma once

#include <string>

#include <json/value.h>

#include <json_config/variable_map.hpp>

namespace components {

struct EventThreadPoolConfig {
  std::string name;

  size_t threads = 2;
  std::string thread_name;

  static EventThreadPoolConfig ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace components
