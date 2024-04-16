#include <components/component_context_impl.hpp>

#include <algorithm>
#include <queue>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>

#include <components/component_context_component_info.hpp>
#include <components/impl/component_name_from_info.hpp>
#include <components/manager.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace components::impl {

namespace {
const std::string kOnAllComponentsLoadedRootName = "all_components_loaded";
const std::string kClearComponentsRootName = "clear_components";

const std::chrono::seconds kPrintAddingComponentsPeriod{10};

template <class Container>
std::string JoinNamesFromInfo(const Container& container,
                              std::string_view separator) {
  std::string chain;
  constexpr std::size_t kAverageComponentNameLength = 32;
  chain.reserve(container.size() * kAverageComponentNameLength);
  for (auto v : container) {
    if (!chain.empty()) {
      chain += separator;
    }
    chain += v.StringViewName();
  }

  return chain;
}

}  // namespace

ComponentContextImpl::TaskToComponentMapScope::TaskToComponentMapScope(
    ComponentContextImpl& context, impl::ComponentNameFromInfo component_name)
    : context_(context) {
  auto data = context_.shared_data_.Lock();
  auto res = data->task_to_component_map.emplace(
      &engine::current_task::GetCurrentTaskContext(), component_name);
  if (!res.second)
    throw std::runtime_error(
        "can't create multiple components in the same task simultaneously: "
        "component " +
        std::string{res.first->second.StringViewName()} +
        " is already registered for current task");
}

ComponentContextImpl::TaskToComponentMapScope::~TaskToComponentMapScope() {
  auto data = context_.shared_data_.Lock();
  data->task_to_component_map.erase(
      &engine::current_task::GetCurrentTaskContext());
}

ComponentContextImpl::SearchingComponentScope::SearchingComponentScope(
    const ComponentContextImpl& context,
    impl::ComponentNameFromInfo component_name)
    : context_(context), component_name_(component_name) {
  auto data = context_.shared_data_.Lock();
  data->searching_components.insert(component_name_);
}

ComponentContextImpl::SearchingComponentScope::~SearchingComponentScope() {
  auto data = context_.shared_data_.Lock();
  data->searching_components.erase(component_name_);
}

ComponentContextImpl::ComponentContextImpl(
    const Manager& manager, std::vector<std::string>&& loading_component_names)
    : manager_(manager) {
  UASSERT(std::is_sorted(loading_component_names.begin(),
                         loading_component_names.end()));
  UASSERT(std::unique(loading_component_names.begin(),
                      loading_component_names.end()) ==
          loading_component_names.end());

  ComponentMap nodes_provider;
  components_.reserve(loading_component_names.size());

  for (auto& component_name : loading_component_names) {
    auto [it, success] = nodes_provider.emplace(impl::ComponentNameFromInfo{},
                                                std::move(component_name));
    UINVARIANT(success, "Failed to emplace component info");

    auto node = nodes_provider.extract(it);
    UASSERT(node);
    auto name = node.mapped().Name();
    node.key() = name;

    components_.insert(std::move(node));
  }

  StartPrintAddingComponentsTask();
}

impl::ComponentBase* ComponentContextImpl::AddComponent(
    std::string_view name, const ComponentFactory& factory,
    ComponentContext& context) {
  auto& component_info = components_.at(impl::ComponentNameFromInfo{name});
  TaskToComponentMapScope task_to_component_map_scope(*this,
                                                      component_info.Name());

  if (component_info.GetComponent())
    throw std::runtime_error("trying to add component " + std::string{name} +
                             " multiple times");

  component_info.SetComponent(factory(context));
  auto* component = component_info.GetComponent();
  if (component) {
    // Call the following command on logs to get the component dependencies:
    // sed -n 's/^.*component deps: \(.*\)$/\1/p'
    LOG_TRACE() << "component deps: "
                << fmt::format("\"{0}\" [label=\"{0}\n{1}\"]; ", name,
                               compiler::GetTypeName(typeid(*component)))
                << component_info.GetDependencies();
  }
  return component;
}

void ComponentContextImpl::OnAllComponentsLoaded() {
  StopPrintAddingComponentsTask();
  tracing::Span span(kOnAllComponentsLoadedRootName);
  return ProcessAllComponentLifetimeStageSwitchings(
      {impl::ComponentLifetimeStage::kRunning,
       &impl::ComponentInfo::OnAllComponentsLoaded, "OnAllComponentsLoaded()",
       DependencyType::kNormal, true});
}

void ComponentContextImpl::OnAllComponentsAreStopping() {
  LOG_INFO() << "Sending stopping notification to all components";
  ProcessAllComponentLifetimeStageSwitchings(
      {impl::ComponentLifetimeStage::kReadyForClearing,
       &impl::ComponentInfo::OnAllComponentsAreStopping,
       "OnAllComponentsAreStopping()", DependencyType::kInverted, false});
}

void ComponentContextImpl::ClearComponents() {
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

engine::TaskProcessor& ComponentContextImpl::GetTaskProcessor(
    const std::string& name) const {
  const auto& task_processor_map = manager_.GetTaskProcessorsMap();
  auto it = task_processor_map.find(name);
  if (it == task_processor_map.cend()) {
    throw std::runtime_error("Failed to find task processor with name: " +
                             name);
  }
  return *it->second;
}

const Manager& ComponentContextImpl::GetManager() const { return manager_; }

void ComponentContextImpl::CancelComponentsLoad() {
  CancelComponentLifetimeStageSwitching();
  if (components_load_cancelled_.test_and_set()) return;
  for (auto& component_item : components_) {
    component_item.second.OnLoadingCancelled();
  }
}

bool ComponentContextImpl::IsAnyComponentInFatalState() const {
#ifndef NDEBUG
  {
    const auto data = shared_data_.Lock();
    UASSERT_MSG(data->print_adding_components_stopped,
                "IsAnyComponentInFatalState() should be called only after all "
                "the components has been loaded.");
  }
#endif

  for (const auto& [name, comp] : components_) {
    switch (comp.GetComponent()->GetComponentHealth()) {
      case ComponentHealth::kFatal:
        LOG_ERROR() << "Component '" << name << "' is in kFatal state";
        return true;
      case ComponentHealth::kFallback:
        LOG_LIMITED_WARNING()
            << "Component '" << name << "' is in kFallback state";
        break;
      case ComponentHealth::kOk:
        LOG_DEBUG() << "Component '" << name << "' is in kOk state";
        break;
    }
  }

  return false;
}

bool ComponentContextImpl::HasDependencyOn(std::string_view component_name,
                                           std::string_view dependency) const {
  impl::ComponentNameFromInfo from{dependency};
  impl::ComponentNameFromInfo to{component_name};

  if (!Contains(component_name)) {
    UASSERT_MSG(false,
                fmt::format("Exception while calling HasDependencyOn(\"{0}\", "
                            "\"{1}\"). Component \"{0}\" was not loaded. Only "
                            "dependency component is allowed to not be loaded.",
                            component_name, dependency));
    return false;
  }

  if (!Contains(dependency)) {
    return false;
  }

  if (component_name == dependency) {
    return false;
  }

  const auto data = shared_data_.Lock();
  UASSERT_MSG(data->print_adding_components_stopped,
              "HasDependencyOn() should be called only after all the "
              "components has been loaded.");

  std::set<impl::ComponentNameFromInfo> handled;
  return FindDependencyPathDfs(from, to, handled, nullptr, *data);
}

std::unordered_set<std::string_view> ComponentContextImpl::GetAllDependencies(
    std::string_view component_name) const {
  impl::ComponentNameFromInfo from{component_name};

  UASSERT_MSG(Contains(component_name),
              fmt::format("Exception while calling GetAllDependencies(\"{0}\" "
                          "). Component \"{0}\" was not loaded.",
                          component_name));

  const auto data = shared_data_.Lock();
  UASSERT_MSG(data->print_adding_components_stopped,
              "HasDependencyOn() should be called only after all the "
              "components has been loaded.");

  std::unordered_set<impl::ComponentNameFromInfo> handled;
  FindAllDependenciesImpl(from, handled, *data);

  std::unordered_set<std::string_view> result;
  result.reserve(handled.size());
  for (auto value : handled) {
    if (from != value) {
      result.insert(value.GetUnderlying());
    }
  }
  return result;
}

bool ComponentContextImpl::Contains(std::string_view name) const noexcept {
  return components_.count(impl::ComponentNameFromInfo{name}) != 0;
}

void ComponentContextImpl::ThrowNonRegisteredComponent(
    std::string_view name, std::string_view type) const {
  auto data = shared_data_.Lock();
  throw std::runtime_error(fmt::format(
      "Component '{}' requested component {} with name '{}'. That name is "
      "missing in the static config or the '{}' static config section contains "
      "'load-enabled: false'.",
      GetLoadingComponentName(*data).StringViewName(), type, name, name));
}

void ComponentContextImpl::ThrowComponentTypeMismatch(
    std::string_view name, std::string_view type,
    impl::ComponentBase* component) const {
  auto data = shared_data_.Lock();
  throw std::runtime_error(fmt::format(
      "Component '{}' requested component with name '{}' that is actually "
      "{}{} rather than a {}",
      GetLoadingComponentName(*data).StringViewName(), name,
      (component ? "has type " : "a nullptr"),
      (component ? compiler::GetTypeName(typeid(*component)) : std::string{}),
      type));
}

void ComponentContextImpl::ProcessSingleComponentLifetimeStageSwitching(
    impl::ComponentNameFromInfo name, impl::ComponentInfo& component_info,
    ComponentLifetimeStageSwitchingParams& params) {
  LOG_DEBUG() << "Preparing to call " << params.stage_switch_handler_name
              << " for component " << name.StringViewName();

  auto wait_cb = [&](impl::ComponentNameFromInfo component_name) {
    auto& dependency_from =
        (params.dependency_type == DependencyType::kNormal ? name
                                                           : component_name);
    auto& dependency_to =
        (params.dependency_type == DependencyType::kInverted ? name
                                                             : component_name);
    auto& other_component_info = components_.at(component_name);
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

    LOG_DEBUG() << "Call " << params.stage_switch_handler_name
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

void ComponentContextImpl::ProcessAllComponentLifetimeStageSwitchings(
    ComponentLifetimeStageSwitchingParams params) {
  PrepareComponentLifetimeStageSwitching();

  std::vector<
      std::pair<impl::ComponentNameFromInfo, engine::TaskWithResult<void>>>
      tasks;
  tasks.reserve(components_.size());
  for (auto& component_item : components_) {
    const auto& name = component_item.first;
    auto& component_info = component_item.second;
    tasks.emplace_back(name, engine::CriticalAsyncNoSpan([&]() {
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

impl::ComponentBase* ComponentContextImpl::DoFindComponent(
    std::string_view name) {
  auto& component_info = components_.at(impl::ComponentNameFromInfo{name});
  AddDependency(component_info.Name());

  auto* component = component_info.GetComponent();
  if (component) return component;

  impl::ComponentNameFromInfo this_component_name;

  {
    engine::TaskCancellationBlocker block_cancel;
    auto data = shared_data_.Lock();
    this_component_name = GetLoadingComponentName(*data);
    LOG_DEBUG() << "component " << name << " is not loaded yet, component "
                << this_component_name << " is waiting for it to load";
  }
  SearchingComponentScope finder(*this, this_component_name);

  return component_info.WaitAndGetComponent();
}

void ComponentContextImpl::AddDependency(impl::ComponentNameFromInfo name) {
  auto data = shared_data_.Lock();

  const auto current_component_name = GetLoadingComponentName(*data);
  if (components_.at(current_component_name).CheckItDependsOn(name)) return;

  LOG_DEBUG() << "Resolving dependency " << current_component_name << " -> "
              << name;
  CheckForDependencyCycle(current_component_name, name, *data);

  components_.at(current_component_name).AddItDependsOn(name);
  components_.at(name).AddDependsOnIt(current_component_name);
}

bool ComponentContextImpl::FindDependencyPathDfs(
    impl::ComponentNameFromInfo current, impl::ComponentNameFromInfo target,
    std::set<impl::ComponentNameFromInfo>& handled,
    std::vector<impl::ComponentNameFromInfo>* dependency_path,
    const ProtectedData& data) const {
  handled.insert(current);
  bool found = (current == target);

  components_.at(current).ForEachDependsOnIt(
      [&](impl::ComponentNameFromInfo name) {
        if (!found && !handled.count(name))
          found = FindDependencyPathDfs(name, target, handled, dependency_path,
                                        data);
      });

  if (found && dependency_path) dependency_path->push_back(current);

  return found;
}

void ComponentContextImpl::FindAllDependenciesImpl(
    impl::ComponentNameFromInfo current,
    std::unordered_set<impl::ComponentNameFromInfo>& handled,
    const ProtectedData& data) const {
  handled.insert(current);

  components_.at(current).ForEachItDependsOn(
      [&](impl::ComponentNameFromInfo name) {
        if (!handled.count(name)) {
          FindAllDependenciesImpl(name, handled, data);
        }
      });
}

void ComponentContextImpl::CheckForDependencyCycle(
    impl::ComponentNameFromInfo new_dependency_from,
    impl::ComponentNameFromInfo new_dependency_to,
    const ProtectedData& data) const {
  std::set<impl::ComponentNameFromInfo> handled;
  std::vector<impl::ComponentNameFromInfo> dependency_chain;

  if (FindDependencyPathDfs(new_dependency_from, new_dependency_to, handled,
                            &dependency_chain, data)) {
    dependency_chain.push_back(new_dependency_to);
    LOG_ERROR() << "Found circular dependency between components: "
                << JoinNamesFromInfo(dependency_chain, " -> ");
    throw std::runtime_error("circular components dependency");
  }
}

void ComponentContextImpl::PrepareComponentLifetimeStageSwitching() {
  for (auto& component_item : components_) {
    component_item.second.SetStageSwitchingCancelled(false);
  }
}

void ComponentContextImpl::CancelComponentLifetimeStageSwitching() {
  for (auto& component_item : components_) {
    component_item.second.SetStageSwitchingCancelled(true);
  }
}

impl::ComponentNameFromInfo ComponentContextImpl::GetLoadingComponentName(
    const ProtectedData& data) {
  try {
    return data.task_to_component_map.at(
        &engine::current_task::GetCurrentTaskContext());
  } catch (const std::exception&) {
    throw std::runtime_error(
        "FindComponent() can be called only from a task of component creation");
  }
}

void ComponentContextImpl::StartPrintAddingComponentsTask() {
  print_adding_components_task_ = engine::CriticalAsyncNoSpan([this]() {
    for (;;) {
      {
        auto data = shared_data_.UniqueLock();
        print_adding_components_cv_.WaitFor(
            data.GetLock(), kPrintAddingComponentsPeriod,
            [&data]() { return data->print_adding_components_stopped; });
        if (data->print_adding_components_stopped) return;
      }
      PrintAddingComponents();
    }
  });
}

void ComponentContextImpl::StopPrintAddingComponentsTask() {
  LOG_DEBUG() << "Stopping adding components printing";
  {
    auto data = shared_data_.Lock();
    data->print_adding_components_stopped = true;
  }
  print_adding_components_cv_.NotifyAll();
  print_adding_components_task_ = {};
}

void ComponentContextImpl::PrintAddingComponents() const {
  std::vector<impl::ComponentNameFromInfo> adding_components;
  std::vector<impl::ComponentNameFromInfo> busy_components;

  {
    auto data = shared_data_.Lock();
    adding_components.reserve(data->task_to_component_map.size());
    for (const auto& elem : data->task_to_component_map) {
      const auto& name = elem.second;
      adding_components.push_back(name);
      if (data->searching_components.count(name) == 0)
        busy_components.push_back(name);
    }
  }
  LOG_INFO() << "still adding components, busy: ["
             << JoinNamesFromInfo(busy_components, ", ") << "], loading: ["
             << JoinNamesFromInfo(adding_components, ", ") << ']';
}

}  // namespace components::impl

USERVER_NAMESPACE_END
