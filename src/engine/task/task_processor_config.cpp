#include "task_processor_config.hpp"

#include <json_config/value.hpp>

namespace engine {

TaskProcessorConfig TaskProcessorConfig::ParseFromJson(
    const Json::Value& json, const std::string& full_path,
    const json_config::VariableMapPtr& config_vars_ptr) {
  if (!json.isObject())
    throw std::runtime_error(
        "task_processor config object not found or is not an Object");

  TaskProcessorConfig config;
  config.name =
      json_config::ParseString(json, "name", full_path, config_vars_ptr);
  config.worker_threads = json_config::ParseUint64(json, "worker_threads",
                                                   full_path, config_vars_ptr);
  config.thread_name =
      json_config::ParseString(json, "thread_name", full_path, config_vars_ptr);
  config.scheduler =
      json_config::ParseString(json, "scheduler", full_path, config_vars_ptr);
  config.task_queue_size_threshold = json_config::ParseUint64(
      json, "task_queue_size_threshold", full_path, config_vars_ptr);
  return config;
}

}  // namespace engine
