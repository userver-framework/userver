#include <engine/task/task_processor_config.hpp>

#include <cstdint>

#include <fmt/format.h>

#include <userver/formats/json/value.hpp>
#include <userver/logging/log.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

TaskProcessorConfig Parse(const yaml_config::YamlConfig& value,
                          formats::parse::To<TaskProcessorConfig>) {
  TaskProcessorConfig config;
  config.should_guess_cpu_limit =
      value["guess-cpu-limit"].As<bool>(config.should_guess_cpu_limit);
  config.worker_threads = value["worker_threads"].As<std::size_t>();
  config.thread_name = value["thread_name"].As<std::string>();

  const auto task_trace = value["task-trace"];
  if (!task_trace.IsMissing()) {
    config.task_trace_every =
        task_trace["every"].As<std::size_t>(config.task_trace_every);
    config.task_trace_max_csw =
        task_trace["max-context-switch-count"].As<std::size_t>(1000);
    config.task_trace_logger_name = task_trace["logger"].As<std::string>();
  }

  return config;
}

void TaskProcessorConfig::SetName(const std::string& new_name) {
  name = new_name;
}

using OverloadAction = TaskProcessorSettings::OverloadAction;

OverloadAction Parse(const formats::json::Value& value,
                     formats::parse::To<OverloadAction>) {
  const auto string = value.As<std::string>();
  if (string == "cancel") {
    return OverloadAction::kCancel;
  } else if (string == "ignore") {
    return OverloadAction::kIgnore;
  } else {
    LOG_ERROR() << fmt::format("Unknown enum value at '{}': '{}'",
                               value.GetPath(), string);
    return OverloadAction::kIgnore;
  }
}

TaskProcessorSettings Parse(const formats::json::Value& value,
                            formats::parse::To<TaskProcessorSettings>) {
  engine::TaskProcessorSettings settings;

  const auto overload_doc = value["wait_queue_overload"];

  settings.wait_queue_time_limit = std::chrono::microseconds(
      overload_doc["time_limit_us"].As<std::int64_t>());
  settings.wait_queue_length_limit =
      overload_doc["length_limit"].As<std::int64_t>();
  settings.sensor_wait_queue_time_limit = std::chrono::microseconds(
      overload_doc["sensor_time_limit_us"].As<std::int64_t>(3000));
  settings.overload_action =
      overload_doc["action"].As<OverloadAction>(OverloadAction::kIgnore);

  return settings;
}

}  // namespace engine

USERVER_NAMESPACE_END
