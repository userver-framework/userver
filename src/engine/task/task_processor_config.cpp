#include "task_processor_config.hpp"

#include <yaml_config/value.hpp>

namespace engine {

TaskProcessorConfig TaskProcessorConfig::ParseFromYaml(
    const formats::yaml::Node& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  TaskProcessorConfig config;
  config.name =
      yaml_config::ParseString(yaml, "name", full_path, config_vars_ptr);
  config.worker_threads = yaml_config::ParseUint64(yaml, "worker_threads",
                                                   full_path, config_vars_ptr);
  config.thread_name =
      yaml_config::ParseString(yaml, "thread_name", full_path, config_vars_ptr);
#ifdef USERVER_PROFILER
  const auto profiler_threshold_us =
      yaml_config::ParseOptionalUint64(yaml, "profiler_threshold_us", full_path,
                                       config_vars_ptr)
          .value_or(500);
#else   // USERVER_PROFILER
  const auto profiler_threshold_us = 0;
#endif  // USERVER_PROFILER
  config.profiler_threshold = std::chrono::microseconds(profiler_threshold_us);

  return config;
}

}  // namespace engine
