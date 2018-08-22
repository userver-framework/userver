#include <yandex/taxi/userver/components/component_context.hpp>

#include <engine/task/task_processor.hpp>

#include <yandex/taxi/userver/logging/log.hpp>

namespace components {

ComponentContext::ComponentContext(const Manager& manager,
                                   TaskProcessorMap task_processor_map)
    : manager_(manager), task_processor_map_(std::move(task_processor_map)) {}

void ComponentContext::AddComponent(
    std::string name, std::unique_ptr<ComponentBase>&& component) {
  components_.emplace(name, std::move(component));
  component_names_.push_back(std::move(name));
}

void ComponentContext::ClearComponents() {
  LOG_TRACE() << "Stopping components";
  for (auto it = component_names_.rbegin(); it != component_names_.rend();
       ++it) {
    LOG_INFO() << "Stopping component " << *it;
    try {
      components_.erase(*it);
    } catch (const std::exception& ex) {
      LOG_CRITICAL() << "Exception while stopping component " << *it << ": "
                     << ex.what();
      LOG_FLUSH();
      abort();
    }
    LOG_INFO() << "Stopped component " << *it;
  }
  LOG_TRACE() << "Stopped all components";
}

void ComponentContext::OnAllComponentsLoaded() {
  for (auto& component_item : components_) {
    component_item.second->OnAllComponentsLoaded();
  }
}

size_t ComponentContext::ComponentCount() const { return components_.size(); }

ComponentContext::ComponentMap::const_iterator ComponentContext::begin() const {
  return components_.cbegin();
}

ComponentContext::ComponentMap::const_iterator ComponentContext::end() const {
  return components_.cend();
}

const Manager& ComponentContext::GetManager() const { return manager_; }

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
