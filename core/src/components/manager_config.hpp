#pragma once

#include <string>
#include <vector>

#include <userver/components/component_config.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <engine/coro/pool_config.hpp>
#include <engine/ev/thread_pool_config.hpp>
#include <engine/task/task_processor_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

enum class ValidationMode;

struct ManagerConfig {
  engine::coro::PoolConfig coro_pool;
  engine::ev::ThreadPoolConfig event_thread_pool;
  std::vector<components::ComponentConfig> components;
  std::vector<engine::TaskProcessorConfig> task_processors;
  std::string default_task_processor;
  ValidationMode validate_components_configs{};
  utils::impl::UserverExperimentSet enabled_experiments;
  bool experiments_force_enabled{false};
  bool mlock_debug_info{true};

  static ManagerConfig FromString(
      const std::string&, const std::optional<std::string>& config_vars_path,
      const std::optional<std::string>& config_vars_override_path);
  static ManagerConfig FromFile(
      const std::string& path,
      const std::optional<std::string>& config_vars_path,
      const std::optional<std::string>& config_vars_override_path);
};

ManagerConfig Parse(const yaml_config::YamlConfig& value,
                    formats::parse::To<ManagerConfig>);

}  // namespace components

USERVER_NAMESPACE_END
