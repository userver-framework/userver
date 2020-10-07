#include <components/component_context.hpp>

#include <queue>

#include <boost/algorithm/string/join.hpp>

#include <components/manager.hpp>
#include <engine/task/cancel.hpp>
#include <engine/task/task_processor.hpp>
#include <logging/log.hpp>
#include <tracing/tracer.hpp>
#include <utils/assert.hpp>
#include <utils/async.hpp>

#include "component_context_component_info.hpp"

namespace components {

namespace {
const std::string kOnAllComponentsLoadedRootName = "all_components_loaded";
const std::string kClearComponentsRootName = "clear_components";

const std::chrono::seconds kPrintAddingComponentsPeriod{10};
}  // namespace

ComponentsLoadCancelledException::ComponentsLoadCancelledException()
    : std::runtime_error("Components load cancelled") {}

ComponentsLoadCancelledException::ComponentsLoadCancelledException(
    const std::string& message)
    : std::runtime_error(message) {}

ComponentContext::TaskToComponentMapScope::TaskToComponentMapScope(
    ComponentContext& context, const std::string& component_name)
    : context_(context) {
  auto data = context_.shared_data_.Lock();
  auto res = data->task_to_component_map.emplace(
      engine::current_task::GetCurrentTaskContext(), component_name);
  if (!res.second)
    throw std::runtime_error(
        "can't create multiple components in the same task simultaneously: "
        "component " +
        res.first->second + " is already registered for current task");
}

ComponentContext::TaskToComponentMapScope::~TaskToComponentMapScope() {
  auto data = context_.shared_data_.Lock();
  data->task_to_component_map.erase(
      engine::current_task::GetCurrentTaskContext());
}

ComponentContext::SearchingComponentScope::SearchingComponentScope(
    const ComponentContext& context, const std::string& component_name)
    : context_(context), component_name_(component_name) {
  auto data = context_.shared_data_.Lock();
  data->searching_components.insert(component_name);
}

ComponentContext::SearchingComponentScope::~SearchingComponentScope() {
  auto data = context_.shared_data_.Lock();
  data->searching_components.erase(component_name_);
}

ComponentContext::ComponentContext(
    const Manager& manager,
    const std::set<std::string>& loading_component_names)
    : manager_(manager) {
  for (const auto& component_name : loading_component_names) {
    components_.emplace(
        std::piecewise_construct, std::tie(component_name),
        std::forward_as_tuple(
            std::make_unique<impl::ComponentInfo>(component_name)));
  }
  StartPrintAddingComponentsTask();
}

ComponentContext::~ComponentContext() = default;

impl::ComponentBase* ComponentContext::AddComponent(
    const std::string& name, const ComponentFactory& factory) {
  TaskToComponentMapScope task_to_component_map_scope(*this, name);

  auto& component_info = *components_.at(name);
  if (component_info.GetComponent())
    throw std::runtime_error("trying to add component " + name +
                             " multiple times");

  component_info.SetComponent(factory(*this));

  return component_info.GetComponent();
}

void ComponentContext::RemoveComponent(const std::string& name) {
  components_.erase(name);
}

void ComponentContext::OnAllComponentsLoaded() {
  StopPrintAddingComponentsTask();
  tracing::Span span(kOnAllComponentsLoadedRootName);
  return ProcessAllComponentLifetimeStageSwitchings(
      {impl::ComponentLifetimeStage::kRunning,
       &impl::ComponentInfo::OnAllComponentsLoaded, "OnAllComponentsLoaded()",
       DependencyType::kNormal, true});
}

void ComponentContext::OnAllComponentsAreStopping() {
  LOG_INFO() << "Sending stopping notification to all components";
  ProcessAllComponentLifetimeStageSwitchings(
      {impl::ComponentLifetimeStage::kReadyForClearing,
       &impl::ComponentInfo::OnAllComponentsAreStopping,
       "OnAllComponentsAreStopping()", DependencyType::kInverted, false});
}

void ComponentContext::ClearComponents() {
  StopPrintAddingComponentsTask();
  tracing::Span span(kClearComponentsRootName);
  OnAllComponentsAreStopping();

  LOG_INFO() << "Stopping components";
  ProcessAllComponentLifetimeStageSwitchings(
      {impl::ComponentLifetimeStage::kNull,
       &impl::ComponentInfo::ClearComponent, "ClearComponent()",
       DependencyType::kInverted, false});

  LOG_INFO() << "Stopped all components";
}

engine::TaskProcessor& ComponentContext::GetTaskProcessor(
    const std::string& name) const {
  const auto& task_processor_map = manager_.GetTaskProcessorsMap();
  auto it = task_processor_map.find(name);
  if (it == task_processor_map.cend()) {
    throw std::runtime_error("Failed to find task processor with name: " +
                             name);
  }
  return *it->second;
}

const Manager& ComponentContext::GetManager() const { return manager_; }

void ComponentContext::CancelComponentsLoad() {
  CancelComponentLifetimeStageSwitching();
  if (components_load_cancelled_.test_and_set()) return;
  for (const auto& component_item : components_) {
    component_item.second->OnLoadingCancelled();
  }
}

void ComponentContext::ProcessSingleComponentLifetimeStageSwitching(
    const std::string& name, impl::ComponentInfo& component_info,
    ComponentLifetimeStageSwitchingParams& params) {
  LOG_DEBUG() << "Preparing to call " << params.stage_switch_handler_name
              << " for component " << name;

  auto wait_cb = [&](const std::string& component_name) {
    auto& dependency_from =
        (params.dependency_type == DependencyType::kNormal ? name
                                                           : component_name);
    auto& dependency_to =
        (params.dependency_type == DependencyType::kInverted ? name
                                                             : component_name);
    auto& other_component_info = *components_.at(component_name);
    if (other_component_info.GetStage() != params.next_stage) {
      LOG_DEBUG() << "Cannot call " << params.stage_switch_handler_name
                  << " for component " << name << " yet (" << dependency_from
                  << " depends on " << dependency_to << ")";
      other_component_info.WaitStage(params.next_stage,
                                     params.stage_switch_handler_name);
    }
  };
  try {
    if (params.dependency_type == DependencyType::kNormal)
      component_info.ForEachItDependsOn(wait_cb);
    else
      component_info.ForEachDependsOnIt(wait_cb);

    LOG_INFO() << "Call " << params.stage_switch_handler_name
               << " for component " << name;
    (component_info.*params.stage_switch_handler)();
  } catch (const impl::StageSwitchingCancelledException& ex) {
    LOG_WARNING() << params.stage_switch_handler_name
                  << " failed for component " << name << ": " << ex;
    component_info.SetStage(params.next_stage);
    throw;
  } catch (const std::exception& ex) {
    LOG_ERROR() << params.stage_switch_handler_name << " failed for component "
                << name << ": " << ex;
    if (params.allow_cancelling) {
      component_info.SetStageSwitchingCancelled(true);
      if (!params.is_component_lifetime_stage_switchings_cancelled.exchange(
              true)) {
        CancelComponentLifetimeStageSwitching();
      }
      component_info.SetStage(params.next_stage);
      throw;
    }
  }
  component_info.SetStage(params.next_stage);
}

void ComponentContext::ProcessAllComponentLifetimeStageSwitchings(
    ComponentLifetimeStageSwitchingParams params) {
  PrepareComponentLifetimeStageSwitching();

  std::vector<std::pair<std::string, engine::TaskWithResult<void>>> tasks;
  for (const auto& component_item : components_) {
    const auto& name = component_item.first;
    auto& component_info = *component_item.second;
    tasks.emplace_back(name, engine::impl::CriticalAsync([&]() {
                         ProcessSingleComponentLifetimeStageSwitching(
                             name, component_info, params);
                       }));
  }

  try {
    for (auto& task_item : tasks) {
      try {
        task_item.second.Get();
      } catch (const impl::StageSwitchingCancelledException& ex) {
      }
    }
  } catch (const std::exception& ex) {
    if (params.allow_cancelling &&
        !params.is_component_lifetime_stage_switchings_cancelled.exchange(
            true)) {
      CancelComponentLifetimeStageSwitching();
    }

    for (auto& task_item : tasks) {
      if (task_item.second.IsValid()) task_item.second.Wait();
    }

    throw;
  }

  if (params.is_component_lifetime_stage_switchings_cancelled)
    throw std::logic_error(
        params.stage_switch_handler_name +
        " cancelled but only StageSwitchingCancelledExceptions were caught");
}

impl::ComponentBase* ComponentContext::DoFindComponent(
    const std::string& name) const {
  AddDependency(name);

  auto& component_info = *components_.at(name);
  auto component = component_info.GetComponent();
  if (component) return component;

  std::string this_component_name;

  {
    engine::TaskCancellationBlocker block_cancel;
    auto data = shared_data_.Lock();
    this_component_name = GetLoadingComponentName(*data);
    LOG_INFO() << "component " << name << " is not loaded yet, component "
               << this_component_name << " is waiting for it to load";
  }
  SearchingComponentScope finder(*this, this_component_name);

  return component_info.WaitAndGetComponent();
}

void ComponentContext::AddDependency(const std::string& name) const {
  auto data = shared_data_.Lock();

  const auto& current_component_name = GetLoadingComponentName(*data);
  if (components_.at(current_component_name)->CheckItDependsOn(name)) return;

  LOG_INFO() << "Resolving dependency " << current_component_name << " -> "
             << name;
  CheckForDependencyCycle(current_component_name, name, *data);

  components_.at(current_component_name)->AddItDependsOn(name);
  components_.at(name)->AddDependsOnIt(current_component_name);
}

bool ComponentContext::FindDependencyPathDfs(
    const std::string& current, const std::string& target,
    std::set<std::string>& handled, std::vector<std::string>& dependency_path,
    const ProtectedData& data) const {
  handled.insert(current);
  bool found = (current == target);

  components_.at(current)->ForEachDependsOnIt([&](const std::string& name) {
    if (!found && !handled.count(name))
      found =
          FindDependencyPathDfs(name, target, handled, dependency_path, data);
  });

  if (found) dependency_path.push_back(current);

  return found;
}

void ComponentContext::CheckForDependencyCycle(
    const std::string& new_dependency_from,
    const std::string& new_dependency_to, const ProtectedData& data) const {
  std::set<std::string> handled;
  std::vector<std::string> dependency_chain;

  if (FindDependencyPathDfs(new_dependency_from, new_dependency_to, handled,
                            dependency_chain, data)) {
    dependency_chain.push_back(new_dependency_to);
    LOG_ERROR() << "Found circular dependency between components: "
                << boost::algorithm::join(dependency_chain, " -> ");
    throw std::runtime_error("circular components dependency");
  }
}

void ComponentContext::PrepareComponentLifetimeStageSwitching() {
  for (const auto& component_item : components_) {
    component_item.second->SetStageSwitchingCancelled(false);
  }
}

void ComponentContext::CancelComponentLifetimeStageSwitching() {
  for (const auto& component_item : components_) {
    component_item.second->SetStageSwitchingCancelled(true);
  }
}

std::string ComponentContext::GetLoadingComponentName(
    const ProtectedData& data) {
  try {
    return data.task_to_component_map.at(
        engine::current_task::GetCurrentTaskContext());
  } catch (const std::exception&) {
    throw std::runtime_error(
        "FindComponent() can be called only from a task of component creation");
  }
}

void ComponentContext::StartPrintAddingComponentsTask() {
  print_adding_components_task_ =
      std::make_unique<engine::TaskWithResult<void>>(
          engine::impl::CriticalAsync([this]() {
            for (;;) {
              {
                auto data = shared_data_.UniqueLock();
                print_adding_components_cv_.WaitFor(
                    data.GetLock(), kPrintAddingComponentsPeriod, [&data]() {
                      return data->print_adding_components_stopped;
                    });
                if (data->print_adding_components_stopped) return;
              }
              PrintAddingComponents();
            }
          }));
}

void ComponentContext::StopPrintAddingComponentsTask() {
  LOG_DEBUG() << "Stopping adding components printing";
  {
    auto data = shared_data_.Lock();
    data->print_adding_components_stopped = true;
  }
  print_adding_components_cv_.NotifyAll();
  print_adding_components_task_.reset();
}

void ComponentContext::PrintAddingComponents() const {
  std::vector<std::string> adding_components;
  std::vector<std::string> busy_components;

  {
    auto data = shared_data_.Lock();
    for (const auto& elem : data->task_to_component_map) {
      const auto& name = elem.second;
      adding_components.push_back(name);
      if (data->searching_components.count(name) == 0)
        busy_components.push_back(name);
    }
  }
  LOG_INFO() << "still adding components, busy: ["
             << boost::algorithm::join(busy_components, ", ") << "], loading: ["
             << boost::algorithm::join(adding_components, ", ") << ']';
}

}  // namespace components
