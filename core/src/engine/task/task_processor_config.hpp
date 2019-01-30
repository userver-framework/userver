#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include <formats/yaml.hpp>

#include <yaml_config/variable_map.hpp>

namespace engine {

struct TaskProcessorConfig {
  std::string name;

  size_t worker_threads = 6;
  std::string thread_name;
  std::chrono::microseconds profiler_threshold;

  size_t task_trace_every = 1000;
  size_t task_trace_max_csw = 0;
  std::string task_trace_logger_name;

  static TaskProcessorConfig ParseFromYaml(
      const formats::yaml::Node& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr);
  void SetName(const std::string& name);
};

}  // namespace engine
