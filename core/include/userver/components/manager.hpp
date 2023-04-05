#pragma once

#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <userver/components/component_context.hpp>
#include <userver/components/component_fwd.hpp>
#include <userver/components/impl/component_base.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {
class TaskProcessorPools;
}  // namespace engine::impl

namespace os_signals {
class ProcessorComponent;
}  // namespace os_signals

namespace components {

class ComponentList;
struct ManagerConfig;

using ComponentConfigMap =
    std::unordered_map<std::string, const ComponentConfig&>;

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
  void AddComponent(const components::ComponentConfigMap& config_map,
                    const std::string& name) {
    // Using std::is_convertible_v because std::is_base_of_v returns true even
    // if ComponentBase is a private, protected, or ambiguous base class.
    static_assert(
        std::is_convertible_v<Component*, components::impl::ComponentBase*>,
        "Component must publicly inherit from "
        "components::LoggableComponentBase");
    AddComponentImpl(config_map, name,
                     [](const components::ComponentConfig& config,
                        const components::ComponentContext& context) {
                       return std::make_unique<Component>(config, context);
                     });
  }

  void OnSignal(int signum);

  std::chrono::steady_clock::time_point GetStartTime() const;

  std::chrono::milliseconds GetLoadDuration() const;

 private:
  class TaskProcessorsStorage {
   public:
    explicit TaskProcessorsStorage(
        std::shared_ptr<engine::impl::TaskProcessorPools> task_processor_pools);
    ~TaskProcessorsStorage();

    void Reset() noexcept;

    using Map = TaskProcessorsMap;

    const Map& GetMap() const { return task_processors_map_; }
    const std::shared_ptr<engine::impl::TaskProcessorPools>&
    GetTaskProcessorPools() const {
      return task_processor_pools_;
    }

    void Add(std::string name,
             std::unique_ptr<engine::TaskProcessor>&& task_processor);

   private:
    std::shared_ptr<engine::impl::TaskProcessorPools> task_processor_pools_;
    Map task_processors_map_;
  };

  void CreateComponentContext(const ComponentList& component_list);
  void AddComponents(const ComponentList& component_list);
  void AddComponentImpl(
      const components::ComponentConfigMap& config_map, const std::string& name,
      std::function<std::unique_ptr<components::impl::ComponentBase>(
          const components::ComponentConfig&,
          const components::ComponentContext&)>
          factory);
  void ClearComponents() noexcept;
  components::ComponentConfigMap MakeComponentConfigMap(
      const ComponentList& component_list);

  std::unique_ptr<const ManagerConfig> config_;
  std::vector<ComponentConfig> empty_configs_;
  TaskProcessorsStorage task_processors_storage_;

  mutable std::shared_timed_mutex context_mutex_;
  components::ComponentContext component_context_;
  bool components_cleared_{false};

  engine::TaskProcessor* default_task_processor_{nullptr};
  const std::chrono::steady_clock::time_point start_time_;
  std::chrono::milliseconds load_duration_{0};

  os_signals::ProcessorComponent* signal_processor_{nullptr};
};

}  // namespace components

USERVER_NAMESPACE_END
