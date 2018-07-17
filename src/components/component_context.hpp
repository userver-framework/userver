#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <engine/task/task_processor.hpp>

#include "component_base.hpp"

namespace components {

class Manager;

class ComponentContext {
 public:
  using ComponentMap =
      std::unordered_map<std::string, std::unique_ptr<ComponentBase>>;
  using TaskProcessorMap =
      std::unordered_map<std::string, std::unique_ptr<engine::TaskProcessor>>;

  ComponentContext(const Manager& manager, TaskProcessorMap);

  void AddComponent(std::string name,
                    std::unique_ptr<ComponentBase>&& component);

  void ClearComponents();

  template <typename T>
  T* FindComponent() const {
    return FindComponent<T>(T::kName);
  }

  template <typename T>
  T* FindComponent(const std::string& name) const {
    return dynamic_cast<T*>(DoFindComponent(name));
  }

  size_t ComponentCount() const;

  ComponentMap::const_iterator begin() const;
  ComponentMap::const_iterator end() const;

  engine::TaskProcessor* GetTaskProcessor(const std::string& name) const;

  const Manager& GetManager() const;

 private:
  ComponentBase* DoFindComponent(const std::string& name) const;

  const Manager& manager_;
  ComponentMap components_;
  std::vector<std::string> component_names_;
  TaskProcessorMap task_processor_map_;
};

}  // namespace components
