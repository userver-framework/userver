#pragma once

#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <formats/json/value.hpp>
#include <logging/component.hpp>
#include <utils/statistics/storage.hpp>

#include "component_config.hpp"
#include "component_context.hpp"
#include "impl/component_base.hpp"

namespace engine {
namespace impl {
class TaskProcessorPools;
}  // namespace impl

class TaskProcessor;

}  // namespace engine

namespace components {

class ComponentList;

struct ManagerConfig;

class Manager final {
 public:
  using TaskProcessorsMap =
      std::unordered_map<std::string, std::unique_ptr<engine::TaskProcessor>>;

  Manager(std::unique_ptr<ManagerConfig>&& config,
          const ComponentList& component_list);
  ~Manager();

  const ManagerConfig& GetConfig() const;
  const std::shared_ptr<engine::impl::TaskProcessorPools>&
  GetTaskProcessorPools() const;
  const TaskProcessorsMap& GetTaskProcessorsMap() const;

  template <typename Component>
  std::enable_if_t<
      std::is_base_of<components::impl::ComponentBase, Component>::value>
  AddComponent(const components::ComponentConfigMap& config_map,
               const std::string& name) {
    AddComponentImpl(config_map, name,
                     [](const components::ComponentConfig& config,
                        const components::ComponentContext& context) {
                       return std::make_unique<Component>(config, context);
                     });
  }

  void OnLogRotate();

  std::chrono::steady_clock::time_point GetStartTime() const;

  std::chrono::milliseconds GetLoadDuration() const;

 private:
  void CreateComponentContext(const ComponentList& component_list);
  void AddComponents(const ComponentList& component_list);
  void AddComponentImpl(
      const components::ComponentConfigMap& config_map, const std::string& name,
      std::function<std::unique_ptr<components::impl::ComponentBase>(
          const components::ComponentConfig&,
          const components::ComponentContext&)>
          factory);
  void ClearComponents() noexcept;

 private:
  std::unique_ptr<const ManagerConfig> config_;
  std::shared_ptr<engine::impl::TaskProcessorPools> task_processor_pools_;
  TaskProcessorsMap task_processors_map_;

  mutable std::shared_timed_mutex context_mutex_;
  std::unique_ptr<components::ComponentContext> component_context_;
  bool components_cleared_;

  engine::TaskProcessor* default_task_processor_;
  const std::chrono::steady_clock::time_point start_time_;
  components::Logging* logging_component_;
  std::chrono::milliseconds load_duration_;
};

}  // namespace components
