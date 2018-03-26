#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <engine/task/task_processor.hpp>
#include <server/request/monitor.hpp>

#include "component_base.hpp"

namespace components {

class ComponentContext {
 public:
  using ComponentMap =
      std::unordered_map<std::string, std::unique_ptr<ComponentBase>>;
  using TaskProcessorMap =
      std::unordered_map<std::string, std::unique_ptr<engine::TaskProcessor>>;

  ComponentContext(TaskProcessorMap,
                   std::unique_ptr<server::request::Monitor>&& server_monitor);
  ~ComponentContext();

  void AddComponent(std::string name,
                    std::unique_ptr<ComponentBase>&& component);

  template <typename T>
  T* FindComponent(const std::string& name) const {
    return dynamic_cast<T*>(DoFindComponent(name));
  }

  size_t ComponentCount() const;

  ComponentMap::const_iterator begin() const;
  ComponentMap::const_iterator end() const;

  const server::request::Monitor& GetServerMonitor() const {
    return *server_monitor_;
  }

  engine::TaskProcessor* GetTaskProcessor(const std::string& name) const;

 private:
  ComponentBase* DoFindComponent(const std::string& name) const;

  ComponentMap components_;
  std::vector<std::string> component_names_;
  TaskProcessorMap task_processor_map_;
  std::unique_ptr<server::request::Monitor> server_monitor_;
};

}  // namespace components
