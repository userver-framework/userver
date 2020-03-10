#include <engine/task/task_processor_config.hpp>

#include <yaml_config/value.hpp>

namespace engine {

TaskProcessorConfig TaskProcessorConfig::ParseFromYaml(
    const formats::yaml::Value& yaml, const std::string& full_path,
    const yaml_config::VariableMapPtr& config_vars_ptr) {
  TaskProcessorConfig config;
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

  auto task_trace_yaml = yaml["task-trace"];
  if (!task_trace_yaml.IsMissing()) {
    auto tt_full_path = full_path + ".task-trace";
    config.task_trace_every =
        yaml_config::ParseOptionalUint64(task_trace_yaml, "every", tt_full_path,
                                         config_vars_ptr)
            .value_or(1000);
    config.task_trace_max_csw = yaml_config::ParseOptionalUint64(
                                    task_trace_yaml, "max-context-switch-count",
                                    tt_full_path, config_vars_ptr)
                                    .value_or(1000);
    config.task_trace_logger_name = yaml_config::ParseString(
        task_trace_yaml, "logger", tt_full_path, config_vars_ptr);
  }

  if (yaml.HasMember("guess-cpu-limit")) {
    config.should_guess_cpu_limit = yaml_config::ParseBool(
        yaml, "guess-cpu-limit", full_path, config_vars_ptr);
  }

  return config;
}

void TaskProcessorConfig::SetName(const std::string& name) {
  this->name = name;
}

}  // namespace engine
