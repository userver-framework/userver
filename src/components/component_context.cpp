#include "component_context.hpp"

#include <logging/log.hpp>

namespace components {

ComponentContext::ComponentContext(
    TaskProcessorMap task_processor_map,
    std::unique_ptr<server::request::Monitor>&& server_monitor)
    : task_processor_map_(std::move(task_processor_map)),
      server_monitor_(std::move(server_monitor)) {}

ComponentContext::~ComponentContext() {
  LOG_TRACE() << "Stopping components";
  for (auto it = component_names_.rbegin(); it != component_names_.rend();
       ++it) {
    LOG_INFO() << "Stopping component " << *it;
    components_.erase(*it);
    LOG_INFO() << "Stopped component " << *it;
  }
  LOG_TRACE() << "Stopped all components";
}

void ComponentContext::AddComponent(
    std::string name, std::unique_ptr<ComponentBase>&& component) {
  components_.emplace(name, std::move(component));
  component_names_.push_back(std::move(name));
}

size_t ComponentContext::ComponentCount() const { return components_.size(); }

ComponentContext::ComponentMap::const_iterator ComponentContext::begin() const {
  return components_.cbegin();
}

ComponentContext::ComponentMap::const_iterator ComponentContext::end() const {
  return components_.cend();
}

ComponentBase* ComponentContext::DoFindComponent(
    const std::string& name) const {
  const auto it = components_.find(name);
  if (it == components_.cend()) {
    return nullptr;
  }
  return it->second.get();
}

engine::TaskProcessor* ComponentContext::GetTaskProcessor(
    const std::string& name) const {
  auto it = task_processor_map_.find(name);
  if (it == task_processor_map_.cend()) {
    return nullptr;
  }
  return it->second.get();
}

}  // namespace components
