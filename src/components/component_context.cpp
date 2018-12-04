#include <components/component_context.hpp>

#include <queue>

#include <boost/algorithm/string/join.hpp>

#include <engine/task/task_processor.hpp>
#include <logging/log.hpp>
#include <tracing/tracer.hpp>
#include <utils/async.hpp>

namespace components {

namespace {
const std::string kStopComponentRootName = "all_components_stop";
const std::string kStoppingComponent = "component_stopping";
const std::string kStopComponentName = "component_stop";
const std::string kComponentName = "component_name";
}  // namespace

ComponentsLoadCancelledException::ComponentsLoadCancelledException()
    : std::runtime_error("Components load cancelled") {}

ComponentsLoadCancelledException::ComponentsLoadCancelledException(
    const std::string& message)
    : std::runtime_error(message) {}

ComponentContext::TaskToComponentMapScope::TaskToComponentMapScope(
    ComponentContext& context, const std::string& component_name)
    : context_(context) {
  std::lock_guard<engine::Mutex> lock(context_.component_mutex_);
  auto res = context_.task_to_component_map_.emplace(
      engine::current_task::GetCurrentTaskContext(), component_name);
  if (!res.second)
    throw std::runtime_error(
        "can't create multiple components in the same task simultaneously: "
        "component " +
        res.first->second + " is already registered for current task");
}

ComponentContext::TaskToComponentMapScope::~TaskToComponentMapScope() {
  std::lock_guard<engine::Mutex> lock(context_.component_mutex_);
  context_.task_to_component_map_.erase(
      engine::current_task::GetCurrentTaskContext());
}

ComponentContext::ComponentContext(
    const Manager& manager, TaskProcessorMap task_processor_map,
    std::set<std::string> loading_component_names)
    : manager_(manager),
      task_processor_map_(std::move(task_processor_map)),
      loading_component_names_(std::move(loading_component_names)) {}

ComponentBase* ComponentContext::AddComponent(std::string name,
                                              const ComponentFactory& factory) {
  TaskToComponentMapScope task_to_component_map_scope(*this, name);

  auto component = factory(*this);
  auto pcomponent = component.get();
  {
    std::lock_guard<engine::Mutex> lock(component_mutex_);

    auto res = components_.emplace(std::move(name), std::move(component));
    if (!res.second)
      throw std::runtime_error("duplicate component name: " + res.first->first);
  }

  component_cv_.NotifyAll();
  return pcomponent;
}

void ComponentContext::ClearComponents() {
  tracing::Span span(kStopComponentRootName);
  LOG_INFO() << "Sending stopping notification to all components";
  OnAllComponentsAreStopping();
  LOG_INFO() << "Stopping components";

  std::vector<engine::TaskWithResult<void>> unload_tasks;
  {
    std::lock_guard<engine::Mutex> lock(component_mutex_);
    for (const auto& component_item : components_) {
      const auto& name = component_item.first;
      unload_tasks.emplace_back(utils::CriticalAsync(name, [this, name]() {
        // TODO: create span here with parent task's span as a parent
        WaitAndUnloadComponent(name);
      }));
    }
  }

  for (auto& task : unload_tasks) task.Get();

  LOG_INFO() << "Stopped all components";
}

void ComponentContext::OnAllComponentsAreStopping() {
  std::lock_guard<engine::Mutex> lock(component_mutex_);

  for (auto& component_item : components_) {
    try {
      tracing::Span span(kStoppingComponent);
      component_item.second->OnAllComponentsAreStopping();
    } catch (const std::exception& e) {
      const auto& name = component_item.first;
      LOG_ERROR() << "Exception while sendind stop notification to component " +
                         name + ": " + e.what();
    }
  }
}

void ComponentContext::OnAllComponentsLoaded() {
  std::lock_guard<engine::Mutex> lock(component_mutex_);
  for (auto& component_item : components_) {
    try {
      component_item.second->OnAllComponentsLoaded();
    } catch (const std::exception& ex) {
      std::string message = "OnAllComponentsLoaded() failed for component " +
                            component_item.first + ": " + ex.what();
      LOG_ERROR() << message;
      throw std::runtime_error(message);
    }
  }
}

size_t ComponentContext::ComponentCount() const {
  std::lock_guard<engine::Mutex> lock(component_mutex_);
  return components_.size();
}

const Manager& ComponentContext::GetManager() const { return manager_; }

ComponentBase* ComponentContext::DoFindComponentNoWait(
    const std::string& name, std::unique_lock<engine::Mutex>&) const {
  const auto it = components_.find(name);
  if (it != components_.cend()) {
    return it->second.get();
  }
  return nullptr;
}

void ComponentContext::CheckForDependencyLoop(
    const std::string& entry_name, std::unique_lock<engine::Mutex>&) const {
  std::queue<std::string> todo;
  std::set<std::string> handled;
  std::unordered_map<std::string, std::string> parent;

  todo.push(entry_name);
  handled.insert(entry_name);
  parent[entry_name] = std::string();

  while (!todo.empty()) {
    const auto cur_name = std::move(todo.front());
    todo.pop();

    if (component_dependencies_.count(cur_name) > 0) {
      for (const auto& name : component_dependencies_[cur_name]) {
        if (name == entry_name) {
          std::vector<std::string> dependency_chain;
          dependency_chain.push_back(name);
          for (auto it = cur_name; !it.empty(); it = parent[it])
            dependency_chain.push_back(it);

          LOG_ERROR() << "Found circular dependency between components: "
                      << boost::algorithm::join(dependency_chain, " -> ");
          throw std::runtime_error("circular dependency");
        }

        if (handled.count(name) > 0) continue;

        todo.push(name);
        handled.insert(name);
        parent.emplace(name, cur_name);
      }
    }
  }
}

void ComponentContext::CancelComponentsLoad() {
  std::lock_guard<engine::Mutex> lock(component_mutex_);
  components_load_cancelled_ = true;

  component_cv_.NotifyAll();
}

std::string ComponentContext::GetLoadingComponentName(
    std::unique_lock<engine::Mutex>&) const {
  try {
    return task_to_component_map_.at(
        engine::current_task::GetCurrentTaskContext());
  } catch (const std::exception&) {
    throw std::runtime_error(
        "FindComponent() can be called only from a task of component load");
  }
}

void ComponentContext::AddDependency(const std::string& name) const {
  std::unique_lock<engine::Mutex> lock(component_mutex_);

  const auto& current_component = GetLoadingComponentName(lock);

  auto& current_component_dependency =
      component_dependencies_[current_component];

  if (current_component_dependency.count(name) == 0) {
    LOG_INFO() << "Resolving dependency " << current_component << " -> "
               << name;

    current_component_dependency.insert(name);
    CheckForDependencyLoop(name, lock);
  }
}

void ComponentContext::RemoveComponentDependencies(const std::string& name) {
  LOG_INFO() << "Removing " << name << " from component dependees";
  std::lock_guard<engine::Mutex> lock(component_mutex_);
  component_dependencies_.erase(name);

  component_cv_.NotifyAll();
}

ComponentBase* ComponentContext::DoFindComponent(
    const std::string& name) const {
  AddDependency(name);

  std::unique_lock<engine::Mutex> lock(component_mutex_);

  if (loading_component_names_.count(name) == 0)
    throw std::runtime_error("Requested non-existing component " + name);

  auto component = DoFindComponentNoWait(name, lock);
  if (component) return component;

  LOG_INFO() << "component " << name << " is not loaded yet, component "
             << GetLoadingComponentName(lock) << " is waiting for it to load";

  component_cv_.Wait(lock, [this, &lock, &component, &name]() {
    if (components_load_cancelled_) return true;
    component = DoFindComponentNoWait(name, lock);
    return component != nullptr;
  });

  if (components_load_cancelled_) throw ComponentsLoadCancelledException();
  return component;
}

engine::TaskProcessor* ComponentContext::GetTaskProcessor(
    const std::string& name) const {
  auto it = task_processor_map_.find(name);
  if (it == task_processor_map_.cend()) {
    return nullptr;
  }
  return it->second.get();
}

ComponentContext::TaskProcessorPtrMap ComponentContext::GetTaskProcessorsMap()
    const {
  TaskProcessorPtrMap result;
  for (const auto& it : task_processor_map_)
    result.emplace(it.first, it.second.get());

  return result;
}

bool ComponentContext::MayUnload(const std::string& name) const {
  // If there is any component that depends on 'name' then no
  for (const auto& it : component_dependencies_) {
    const auto& deps = it.second;
    if (deps.count(name) > 0) {
      LOG_INFO() << "Component " << name << " may not unload yet (" << it.first
                 << " depends on " << name << ")";
      return false;
    }
  }

  LOG_INFO() << "Component " << name << " may unload now";
  return true;
}

void ComponentContext::WaitAndUnloadComponent(const std::string& name) {
  std::unique_ptr<ComponentBase> tmp;

  LOG_INFO() << "Preparing to stop component " << name;

  {
    std::unique_lock<engine::Mutex> lock(component_mutex_);
    component_cv_.Wait(lock, [this, &name]() { return MayUnload(name); });

    std::swap(tmp, components_[name]);
    components_.erase(name);
  }

  // Actual stop without the mutex
  {
    tracing::Span span(kStopComponentName);
    span.AddTag(kComponentName, name);

    LOG_INFO() << "Stopping component";
    tmp.reset();
    LOG_INFO() << "Stopped component";
  }

  RemoveComponentDependencies(name);
}

}  // namespace components
