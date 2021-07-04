#pragma once

#include <string>
#include <vector>

#include <userver/components/component_config.hpp>
#include <userver/yaml_config/yaml_config.hpp>

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

  yaml_config::YamlConfig source;

  static ManagerConfig FromString(const std::string&);
  static ManagerConfig FromFile(const std::string& path);
};

ManagerConfig Parse(const yaml_config::YamlConfig& value,
                    formats::parse::To<ManagerConfig>);

}  // namespace components
