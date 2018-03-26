#pragma once

#include <string>

#include <json/value.h>

#include <json_config/variable_map.hpp>

namespace server {

struct SchedulerConfig {
  std::string name;

  size_t threads = 2;
  std::string thread_name;

  static SchedulerConfig ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace server
