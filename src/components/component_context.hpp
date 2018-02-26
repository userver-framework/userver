#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <engine/task/task_processor.hpp>

#include "component_base.hpp"

namespace components {

class ComponentContext {
 public:
  using ComponentMap =
      std::unordered_map<std::string, std::unique_ptr<ComponentBase>>;
  using TaskProcessorMap =
      std::unordered_map<std::string, std::unique_ptr<engine::TaskProcessor>>;

  explicit ComponentContext(TaskProcessorMap);

  void AddComponent(std::string name,
                    std::unique_ptr<ComponentBase>&& component);
  void RemoveComponent(const std::string& name);

  template <typename T>
  T* FindComponent(const std::string& name) const {
    return dynamic_cast<T*>(DoFindComponent(name));
  }

  size_t ComponentCount() const;

  ComponentMap::const_iterator begin() const;
  ComponentMap::const_iterator end() const;

  engine::TaskProcessor* GetTaskProcessor(const std::string& name) const;

 private:
  ComponentBase* DoFindComponent(const std::string& name) const;

  ComponentMap components_;
  TaskProcessorMap task_processor_map_;
};

}  // namespace components
