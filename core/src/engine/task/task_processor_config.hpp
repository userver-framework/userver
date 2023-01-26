#pragma once

#include <chrono>
#include <cstddef>
#include <string>

#include <userver/formats/json_fwd.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

enum class OsScheduling {
  kNormal,
  kLowPriority,
  kIdle,
};

OsScheduling Parse(const yaml_config::YamlConfig& value,
                   formats::parse::To<OsScheduling>);

struct TaskProcessorConfig {
  std::string name;

  bool should_guess_cpu_limit{false};
  std::size_t worker_threads{6};
  std::string thread_name;
  OsScheduling os_scheduling{OsScheduling::kNormal};

  std::size_t task_trace_every{1000};
  std::size_t task_trace_max_csw{0};
  std::string task_trace_logger_name;

  void SetName(const std::string& new_name);
};

TaskProcessorConfig Parse(const yaml_config::YamlConfig& value,
                          formats::parse::To<TaskProcessorConfig>);

struct TaskProcessorSettings {
  std::size_t wait_queue_length_limit{0};
  std::chrono::microseconds wait_queue_time_limit{0};
  std::chrono::microseconds sensor_wait_queue_time_limit{0};

  enum class OverloadAction { kCancel, kIgnore };
  OverloadAction overload_action{OverloadAction::kIgnore};

  std::chrono::microseconds profiler_execution_slice_threshold{0};
  bool profiler_force_stacktrace{false};
};

TaskProcessorSettings::OverloadAction Parse(
    const formats::json::Value& value,
    formats::parse::To<TaskProcessorSettings::OverloadAction>);

TaskProcessorSettings Parse(const formats::json::Value& value,
                            formats::parse::To<TaskProcessorSettings>);

}  // namespace engine

USERVER_NAMESPACE_END
