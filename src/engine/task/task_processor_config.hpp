#pragma once

#include <cstdint>
#include <string>

#include <formats/yaml.hpp>

#include <yaml_config/variable_map.hpp>

namespace engine {

struct TaskProcessorConfig {
  std::string name;

  size_t worker_threads = 6;
  std::string thread_name;

  static TaskProcessorConfig ParseFromYaml(
      const formats::yaml::Node& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace engine
