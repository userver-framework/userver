#include <components/component_context.hpp>

#include <engine/task/task_processor.hpp>
#include <logging/log.hpp>
#include <tracing/tracer.hpp>

namespace components {

namespace {
const std::string kStopComponentRootName = "all_components_stop";
const std::string kStoppingComponent = "component_stopping";
const std::string kStopComponentName = "component_stop";
const std::string kComponentName = "component_name";
}  // namespace

ComponentContext::ComponentContext(const Manager& manager,
                                   TaskProcessorMap task_processor_map)
    : manager_(manager), task_processor_map_(std::move(task_processor_map)) {}

void ComponentContext::AddComponent(
    std::string name, std::unique_ptr<ComponentBase>&& component) {
  components_.emplace(name, std::move(component));
  component_names_.push_back(std::move(name));
}

void ComponentContext::ClearComponents() {
  auto root_span = tracing::Tracer::GetTracer()->CreateSpanWithoutParent(
      kStopComponentRootName);
  TRACE_TRACE(root_span) << "Sending stopping notification to all components";
  OnAllComponentsAreStopping(root_span);
  TRACE_TRACE(root_span) << "Stopping components";

  for (auto it = component_names_.rbegin(); it != component_names_.rend();
       ++it) {
    auto span = root_span.CreateChild(kStopComponentName);
    span.AddTag(kComponentName, *it);

    TRACE_INFO(span) << "Stopping component";
    try {
      components_.erase(*it);
    } catch (const std::exception& ex) {
      TRACE_CRITICAL(span) << "Exception while stopping component: "
                           << ex.what();
      logging::LogFlush();
      abort();
    }
    TRACE_INFO(span) << "Stopped component";
  }
  TRACE_TRACE(root_span) << "Stopped all components";
}

void ComponentContext::OnAllComponentsAreStopping(tracing::Span& parent_span) {
  for (auto& component_item : components_) {
    try {
      auto span = parent_span.CreateChild(kStoppingComponent);
      component_item.second->OnAllComponentsAreStopping();
    } catch (const std::exception& e) {
      const auto& name = component_item.first;
      TRACE_ERROR(parent_span)
          << "Exception while sendind stop notification to component " + name +
                 ": " + e.what();
    }
  }
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
