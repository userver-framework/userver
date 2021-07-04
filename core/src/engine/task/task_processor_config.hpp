#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include <userver/formats/yaml/value.hpp>
#include <userver/yaml_config/yaml_config.hpp>

namespace engine {

struct TaskProcessorConfig {
  std::string name;

  bool should_guess_cpu_limit{false};
  size_t worker_threads{6};
  std::string thread_name;

  size_t task_trace_every{1000};
  size_t task_trace_max_csw{0};
  std::string task_trace_logger_name;

  void SetName(const std::string& name);
};

TaskProcessorConfig Parse(const yaml_config::YamlConfig& value,
                          formats::parse::To<TaskProcessorConfig>);

}  // namespace engine
