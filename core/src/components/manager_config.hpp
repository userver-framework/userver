#pragma once

#include <string>
#include <vector>

#include <components/component_config.hpp>
#include <formats/yaml.hpp>
#include <yaml_config/variable_map.hpp>

#include <engine/coro/pool_config.hpp>
#include <engine/ev/thread_pool_config.hpp>
#include <engine/task/task_processor_config.hpp>

namespace components {

struct ManagerConfig {
  engine::coro::PoolConfig coro_pool;
  engine::ev::ThreadPoolConfig event_thread_pool;
  std::vector<components::ComponentConfig> components;
  std::vector<engine::TaskProcessorConfig> task_processors;
  std::string default_task_processor;

  formats::yaml::Value yaml;  // the owner
  yaml_config::VariableMapPtr config_vars_ptr;

  static ManagerConfig ParseFromYaml(
      formats::yaml::Value yaml, const std::string& name,
      yaml_config::VariableMapPtr config_vars_ptr);

  static ManagerConfig ParseFromString(const std::string&);
  static ManagerConfig ParseFromFile(const std::string& path);
};

}  // namespace components
