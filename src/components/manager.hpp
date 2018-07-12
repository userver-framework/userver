#pragma once

#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <json/value.h>

#include <engine/coro/pool_stats.hpp>
#include <engine/ev/thread_pool.hpp>

#include "component_base.hpp"
#include "component_config.hpp"
#include "component_context.hpp"
#include "manager_config.hpp"

namespace components {

class ComponentList;

enum class MonitorVerbosity { kTerse, kFull };

class Manager {
 public:
  Manager(ManagerConfig config, const ComponentList& component_list);
  ~Manager();

  const ManagerConfig& GetConfig() const;
  engine::coro::PoolStats GetCoroutineStats() const;
  Json::Value GetMonitorData(MonitorVerbosity verbosity) const;

  template <typename Component>
  std::enable_if_t<std::is_base_of<components::ComponentBase, Component>::value>
  AddComponent(const components::ComponentConfigMap& config_map,
               const std::string& name) {
    AddComponentImpl(config_map, name,
                     [](const components::ComponentConfig& config,
                        const components::ComponentContext& context) {
                       return std::make_unique<Component>(config, context);
                     });
  }

  void OnLogRotate();

 private:
  void AddComponentImpl(
      const components::ComponentConfigMap& config_map, const std::string& name,
      std::function<std::unique_ptr<components::ComponentBase>(
          const components::ComponentConfig&,
          const components::ComponentContext&)>
          factory);
  void ClearComponents();

  const ManagerConfig config_;

  std::unique_ptr<engine::TaskProcessor::CoroPool> coro_pool_;
  std::unordered_map<std::string, engine::ev::ThreadPool> event_thread_pools_;

  mutable std::shared_timed_mutex context_mutex_;
  std::unique_ptr<components::ComponentContext> component_context_;
  bool components_cleared_;

  engine::TaskProcessor* default_task_processor_;
};

}  // namespace components
