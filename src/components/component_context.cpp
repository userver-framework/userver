#include "component_context.hpp"

namespace components {

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

}  // namespace components
