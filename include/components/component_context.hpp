#pragma once

#include <atomic>
#include <cassert>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <engine/condition_variable.hpp>
#include <engine/mutex.hpp>
#include <utils/demangle.hpp>
#include "component_base.hpp"

namespace engine {
class TaskProcessor;

namespace impl {
class TaskContext;
}  // namespace impl

}  // namespace engine

namespace tracing {
class Span;
}

namespace components {

class Manager;

class ComponentContext {
 public:
  using ComponentMap =
      std::unordered_map<std::string, std::unique_ptr<ComponentBase>>;
  using TaskProcessorMap =
      std::unordered_map<std::string, std::unique_ptr<engine::TaskProcessor>>;
  using TaskProcessorPtrMap =
      std::unordered_map<std::string, engine::TaskProcessor*>;

  ComponentContext(const Manager& manager, TaskProcessorMap);

  void BeforeAddComponent(std::string name);

  void AddComponent(std::string name,
                    std::unique_ptr<ComponentBase>&& component);

  void ClearComponents();

  void OnAllComponentsLoaded();

  void OnAllComponentsAreStopping(tracing::Span&);

  template <typename T, bool ignore_dependencies = false>
  T& FindComponent() const {
    return FindComponent<T, ignore_dependencies>(T::kName);
  }

  template <typename T, bool ignore_dependencies = false>
  T& FindComponent(const std::string& name) const {
    T* ptr = dynamic_cast<T*>(ignore_dependencies
                                  ? DoFindComponentIgnoreDependencies(name)
                                  : DoFindComponent(name));
    assert(ptr != nullptr);
    if (!ptr) {
      throw std::runtime_error("Cannot find component of type " +
                               utils::GetTypeName(typeid(T)) + " name=" + name);
    }
    return *ptr;
  }

  template <typename T, bool ignore_dependencies = false>
  T* FindComponentOptional() const {
    return FindComponentOptional<T, ignore_dependencies>(T::kName);
  }

  template <typename T, bool ignore_dependencies = false>
  T* FindComponentOptional(const std::string& name) const {
    return dynamic_cast<T*>(ignore_dependencies
                                ? DoFindComponentIgnoreDependencies(name)
                                : DoFindComponent(name));
  }

  size_t ComponentCount() const;

  engine::TaskProcessor* GetTaskProcessor(const std::string& name) const;

  TaskProcessorPtrMap GetTaskProcessorsMap() const;

  const Manager& GetManager() const;

  void CancelComponentsLoad();

  void SetLoadingComponentNames(std::set<std::string> names);

  void RemoveComponentDependencies(const std::string& name);

 private:
  ComponentBase* DoFindComponent(const std::string& name) const;
  ComponentBase* DoFindComponentIgnoreDependencies(
      const std::string& name) const;

  ComponentBase* DoFindComponentNoWait(const std::string& name,
                                       std::unique_lock<engine::Mutex>&) const;

  void AddDependency(const std::string& name) const;

  void CheckForDependencyLoop(const std::string& entry_name,
                              std::unique_lock<engine::Mutex>&) const;

  bool MayUnload(const std::string& name) const;

  void WaitAndUnloadComponent(tracing::Span&, const std::string& name);

  std::string GetLoadingComponentName(std::unique_lock<engine::Mutex>&) const;

  const Manager& manager_;

  mutable engine::Mutex component_mutex_;
  mutable engine::ConditionVariable component_cv_;
  ComponentMap components_;
  std::set<std::string> loading_component_names_;
  std::vector<std::string> component_names_;
  TaskProcessorMap task_processor_map_;

  // dependee -> dependency
  mutable std::unordered_map<std::string, std::set<std::string>>
      component_dependencies_;
  std::unordered_map<engine::impl::TaskContext*, std::string>
      task_to_component_map_;
  bool all_components_loaded_{false};
  bool clear_components_started_{false};
  bool components_load_cancelled_{false};
};

}  // namespace components
