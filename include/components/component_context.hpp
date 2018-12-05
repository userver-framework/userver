#pragma once

#include <atomic>
#include <cassert>
#include <memory>
#include <set>
#include <stdexcept>
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
}  // namespace tracing

namespace components {

class Manager;

class ComponentsLoadCancelledException : public std::runtime_error {
 public:
  ComponentsLoadCancelledException();
  explicit ComponentsLoadCancelledException(const std::string& message);
};

class ComponentContext {
 public:
  using ComponentMap =
      std::unordered_map<std::string, std::unique_ptr<ComponentBase>>;
  using TaskProcessorMap =
      std::unordered_map<std::string, std::unique_ptr<engine::TaskProcessor>>;
  using TaskProcessorPtrMap =
      std::unordered_map<std::string, engine::TaskProcessor*>;
  using ComponentFactory =
      std::function<std::unique_ptr<components::ComponentBase>(
          const components::ComponentContext&)>;

  ComponentContext(const Manager& manager, TaskProcessorMap,
                   std::set<std::string> loading_component_names);

  ComponentBase* AddComponent(std::string name,
                              const ComponentFactory& factory);

  void ClearComponents();

  void OnAllComponentsLoaded();

  void OnAllComponentsAreStopping();

  template <typename T>
  T& FindComponent() const {
    return FindComponent<T>(T::kName);
  }

  template <typename T>
  T& FindComponent(const std::string& name) const {
    T* ptr = dynamic_cast<T*>(DoFindComponent(name));
    assert(ptr != nullptr);
    if (!ptr) {
      throw std::runtime_error("Cannot find component of type " +
                               utils::GetTypeName(typeid(T)) + " name=" + name);
    }
    return *ptr;
  }

  template <typename T>
  T* FindComponentOptional() const {
    return FindComponentOptional<T>(T::kName);
  }

  template <typename T>
  T* FindComponentOptional(const std::string& name) const {
    return dynamic_cast<T*>(DoFindComponent(name));
  }

  size_t ComponentCount() const;

  engine::TaskProcessor& GetTaskProcessor(const std::string& name) const;

  TaskProcessorPtrMap GetTaskProcessorsMap() const;

  const Manager& GetManager() const;

  void CancelComponentsLoad();

  void RemoveComponentDependencies(const std::string& name);

 private:
  class TaskToComponentMapScope {
   public:
    TaskToComponentMapScope(ComponentContext& context,
                            const std::string& component_name);
    ~TaskToComponentMapScope();

   private:
    ComponentContext& context_;
  };

  ComponentBase* DoFindComponent(const std::string& name) const;

  ComponentBase* DoFindComponentNoWait(const std::string& name,
                                       std::unique_lock<engine::Mutex>&) const;

  void AddDependency(const std::string& name) const;

  void CheckForDependencyLoop(const std::string& entry_name,
                              std::unique_lock<engine::Mutex>&) const;

  bool MayUnload(const std::string& name) const;

  void WaitAndUnloadComponent(const std::string& name);

  std::string GetLoadingComponentName(std::unique_lock<engine::Mutex>&) const;

  const Manager& manager_;

  mutable engine::Mutex component_mutex_;
  mutable engine::ConditionVariable component_cv_;
  ComponentMap components_;
  TaskProcessorMap task_processor_map_;
  std::set<std::string> loading_component_names_;

  // dependee -> dependency
  mutable std::unordered_map<std::string, std::set<std::string>>
      component_dependencies_;
  std::unordered_map<engine::impl::TaskContext*, std::string>
      task_to_component_map_;
  bool components_load_cancelled_{false};
};

}  // namespace components
