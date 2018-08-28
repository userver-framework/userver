#pragma once

#include <cstdint>
#include <string>

#include <json/json.h>

#include <json_config/variable_map.hpp>

namespace engine {

struct TaskProcessorConfig {
  std::string name;

  size_t worker_threads = 6;
  std::string thread_name;
  size_t task_queue_size_threshold = 1000000;

  static TaskProcessorConfig ParseFromJson(
      const Json::Value& json, const std::string& full_path,
      const json_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace engine
