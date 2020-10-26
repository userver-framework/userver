#include <engine/task/task_processor_config.hpp>

namespace engine {

namespace {

#ifdef USERVER_PROFILER
constexpr bool kParseProfilerThreshold = true;
#else
constexpr bool kParseProfilerThreshold = false;
#endif

}  // namespace

TaskProcessorConfig Parse(const yaml_config::YamlConfig& value,
                          formats::parse::To<TaskProcessorConfig>) {
  TaskProcessorConfig config;
  config.should_guess_cpu_limit =
      value["guess-cpu-limit"].As<bool>(config.should_guess_cpu_limit);
  config.worker_threads = value["worker_threads"].As<size_t>();
  config.thread_name = value["thread_name"].As<std::string>();

  if constexpr (kParseProfilerThreshold) {
    config.profiler_threshold = std::chrono::microseconds{
        value["profiler_threshold_us"].As<uint64_t>(500)};
  }

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
