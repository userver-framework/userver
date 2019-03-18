#pragma once

#include <string>

#include <formats/yaml.hpp>

#include <yaml_config/variable_map.hpp>

namespace engine {
namespace ev {

struct ThreadPoolConfig {
  size_t threads = 2;
  std::string thread_name = "event-worker";

  static ThreadPoolConfig ParseFromYaml(
      const formats::yaml::Value& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace ev
}  // namespace engine
