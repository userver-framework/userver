#pragma once

#include <atomic>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include <compiler/demangle.hpp>
#include <components/component_base.hpp>
#include <engine/condition_variable.hpp>
#include <engine/mutex.hpp>
#include <engine/task/task_with_result.hpp>
#include <utils/assert.hpp>

namespace engine {
class TaskProcessor;

namespace impl {
class TaskContext;
}  // namespace impl

}  // namespace engine

namespace components {

class Manager;

namespace impl {

enum class ComponentLifetimeStage;
class ComponentInfo;

}  // namespace impl

class ComponentsLoadCancelledException : public std::runtime_error {
 public:
  ComponentsLoadCancelledException();
  explicit ComponentsLoadCancelledException(const std::string& message);
};

class ComponentContext final {
 public:
  using TaskProcessorMap =
      std::unordered_map<std::string, std::unique_ptr<engine::TaskProcessor>>;
  using TaskProcessorPtrMap =
      std::unordered_map<std::string, engine::TaskProcessor*>;
  using ComponentFactory =
      std::function<std::unique_ptr<components::ComponentBase>(
          const components::ComponentContext&)>;

  ComponentContext(const Manager& manager, TaskProcessorMap,
                   const std::set<std::string>& loading_component_names);
  ~ComponentContext();

  ComponentBase* AddComponent(const std::string& name,
                              const ComponentFactory& factory);

  void OnAllComponentsLoaded();

  void OnAllComponentsAreStopping();

  void ClearComponents();

  /// Can only be called from other component's constructor in a task where that
  /// constructor was called.
  /// May block and asynchronously wait for the creation of the requested
  /// component.
  /// @throw `ComponentsLoadCancelledException` if components loading was
  /// cancelled due to errors in the creation of other component.
  /// @throw `std::runtime_error` if component missing in `component_list` was
  /// requested.
  template <typename T>
  T& FindComponent() const {
    return FindComponent<T>(T::kName);
  }

  template <typename T>
  T& FindComponent(const std::string& name) const {
    if (components_.count(name) == 0) {
      std::unique_lock<engine::Mutex> lock(component_mutex_);
      throw std::runtime_error(
          "Component '" + GetLoadingComponentName(lock) +
          "' requested component with non registered name '" + name +
          "' of type " + compiler::GetTypeName<T>());
    }

    auto* component_base = DoFindComponent(name);
    T* ptr = dynamic_cast<T*>(component_base);
    UASSERT(ptr != nullptr);
    if (!ptr) {
      std::unique_lock<engine::Mutex> lock(component_mutex_);
      throw std::runtime_error(
          "Component '" + GetLoadingComponentName(lock) +
          "' requested component with name '" + name + "' that is actually " +
          (component_base
               ? "has type " + compiler::GetTypeName(typeid(*component_base))
               : std::string{"a nullptr"}) +
          " rather than a " + compiler::GetTypeName<T>());
    }
    return *ptr;
  }

  template <typename T>
  T* FindComponentOptional() const {
    return FindComponentOptional<T>(T::kName);
  }

  template <typename T>
  T* FindComponentOptional(const std::string& name) const {
    if (components_.count(name) == 0) {
      return nullptr;
    }
    return dynamic_cast<T*>(DoFindComponent(name));
  }

  engine::TaskProcessor& GetTaskProcessor(const std::string& name) const;

  TaskProcessorPtrMap GetTaskProcessorsMap() const;

  const Manager& GetManager() const;

  void CancelComponentsLoad();

 private:
  class TaskToComponentMapScope final {
   public:
    TaskToComponentMapScope(ComponentContext& context,
                            const std::string& component_name);
    ~TaskToComponentMapScope();

   private:
    ComponentContext& context_;
  };

  using ComponentMap =
      std::unordered_map<std::string, std::unique_ptr<impl::ComponentInfo>>;

  enum class DependencyType { kNormal, kInverted };

  struct ComponentLifetimeStageSwitchingParams {
    ComponentLifetimeStageSwitchingParams(
        const impl::ComponentLifetimeStage& next_stage,
        void (impl::ComponentInfo::*stage_switch_handler)(),
        const std::string& stage_switch_handler_name,
        DependencyType dependency_type, bool allow_cancelling)
        : next_stage(next_stage),
          stage_switch_handler(stage_switch_handler),
          stage_switch_handler_name(stage_switch_handler_name),
          dependency_type(dependency_type),
          allow_cancelling(allow_cancelling),
          is_component_lifetime_stage_switchings_cancelled{false} {}

    const impl::ComponentLifetimeStage& next_stage;
    void (impl::ComponentInfo::*stage_switch_handler)();
    const std::string& stage_switch_handler_name;
    DependencyType dependency_type;
    bool allow_cancelling;
    std::atomic<bool> is_component_lifetime_stage_switchings_cancelled;
  };

  void ProcessSingleComponentLifetimeStageSwitching(
      const std::string& name, impl::ComponentInfo& component_info,
      ComponentLifetimeStageSwitchingParams& params);

  void ProcessAllComponentLifetimeStageSwitchings(
      ComponentLifetimeStageSwitchingParams params);

  ComponentBase* DoFindComponent(const std::string& name) const;

  void AddDependency(const std::string& name) const;

  bool FindDependencyPathDfs(const std::string& current,
                             const std::string& target,
                             std::set<std::string>& handled,
                             std::vector<std::string>& dependency_path,
                             std::unique_lock<engine::Mutex>&) const;
  void CheckForDependencyCycle(const std::string& new_dependency_from,
                               const std::string& new_dependency_to,
                               std::unique_lock<engine::Mutex>&) const;

  void PrepareComponentLifetimeStageSwitching();
  void CancelComponentLifetimeStageSwitching();

  std::string GetLoadingComponentName(std::unique_lock<engine::Mutex>&) const;
  void StartPrintAddingComponentsTask();
  void StopPrintAddingComponentsTask();
  void PrintAddingComponents() const;

  const Manager& manager_;
  TaskProcessorMap task_processor_map_;

  ComponentMap components_;
  std::atomic_flag components_load_cancelled_ = ATOMIC_FLAG_INIT;

  mutable engine::Mutex component_mutex_;
  std::unordered_map<engine::impl::TaskContext*, std::string>
      task_to_component_map_;
  engine::ConditionVariable print_adding_components_cv_;
  bool print_adding_components_stopped_;
  std::unique_ptr<engine::TaskWithResult<void>> print_adding_components_task_;
};

}  // namespace components
