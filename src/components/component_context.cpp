#include "component_context.hpp"

namespace components {

ComponentContext::ComponentContext(
    TaskProcessorMap task_processor_map,
    std::unique_ptr<server::request::Monitor>&& server_monitor)
    : task_processor_map_(std::move(task_processor_map)),
      server_monitor_(std::move(server_monitor)) {}

void ComponentContext::AddComponent(
    std::string name, std::unique_ptr<ComponentBase>&& component) {
  components_.emplace(std::move(name), std::move(component));
}

void ComponentContext::RemoveComponent(const std::string& name) {
  components_.erase(name);
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
