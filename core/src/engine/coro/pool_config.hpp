#pragma once

#include <string>

#include <formats/yaml.hpp>

#include <yaml_config/variable_map.hpp>

namespace engine {
namespace coro {

struct PoolConfig {
  size_t initial_size = 1000;
  size_t max_size = 10000;

  static PoolConfig ParseFromYaml(
      const formats::yaml::Value& yaml, const std::string& full_path,
      const yaml_config::VariableMapPtr& config_vars_ptr);
};

}  // namespace coro
}  // namespace engine
