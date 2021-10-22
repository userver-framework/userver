#include <engine/task/task_processor_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

TaskProcessorConfig Parse(const yaml_config::YamlConfig& value,
                          formats::parse::To<TaskProcessorConfig>) {
  TaskProcessorConfig config;
  config.should_guess_cpu_limit =
      value["guess-cpu-limit"].As<bool>(config.should_guess_cpu_limit);
  config.worker_threads = value["worker_threads"].As<size_t>();
  config.thread_name = value["thread_name"].As<std::string>();

  const auto task_trace = value["task-trace"];
  if (!task_trace.IsMissing()) {
    config.task_trace_every =
        task_trace["every"].As<size_t>(config.task_trace_every);
    config.task_trace_max_csw =
        task_trace["max-context-switch-count"].As<size_t>(1000);
    config.task_trace_logger_name = task_trace["logger"].As<std::string>();
  }

  return config;
}

void TaskProcessorConfig::SetName(const std::string& name) {
  this->name = name;
}

}  // namespace engine

USERVER_NAMESPACE_END
