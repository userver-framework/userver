#include <components/component_context_impl.hpp>

#include <algorithm>

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>
#include <userver/components/component_context.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/task_builder.hpp>

#include <components/component_context_component_info.hpp>
#include <components/manager.hpp>
#include <components/manager_config.hpp>
#include <engine/task/task_processor.hpp>
#include <logging/tp_logger_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace components::impl {

namespace {

const std::string kOnAllComponentsLoadedRootName = "all_components_loaded";
const std::string kClearComponentsRootName = "clear_components";

}  // namespace

std::string_view ComponentInfoProjection(const utils::impl::MutableWrapper<ComponentInfo>& component_info) {
    return component_info->GetName();
}

ComponentContextImpl::LoadingComponentScope::LoadingComponentScope(
    ComponentContextImpl& context,
    ComponentInfo& component
)
    : context_(context),
      component_(component)
{
    auto data = context_.shared_data_.Lock();
    data->loading_components.insert(component_);
}

ComponentContextImpl::LoadingComponentScope::~LoadingComponentScope() {
    auto data = context_.shared_data_.Lock();
    data->loading_components.erase(component_);
}

ComponentContextImpl::SearchingComponentScope::SearchingComponentScope(
    const ComponentContextImpl& context,
    ComponentInfo& component
)
    : context_(context),
      component_(component)
{
    auto data = context_.shared_data_.Lock();
    data->searching_components.insert(component_);
}

ComponentContextImpl::SearchingComponentScope::~SearchingComponentScope() {
    auto data = context_.shared_data_.Lock();
    data->searching_components.erase(component_);
}

template <typename Self>
auto& ComponentContextImpl::GetComponentInfo(Self& self, std::string_view component_name) {
    if (const auto component_info = utils::impl::ProjectedFindOrNullptrUnsafe(self.components_, component_name)) {
        return *component_info;
    }
    throw std::runtime_error(fmt::format("Non-existent component: {}", component_name));
}

ComponentContextImpl::ComponentContextImpl(const Manager& manager, std::vector<std::string>&& loading_component_names)
    : manager_(manager),
      trace_plugin_(engine::current_task::GetTaskProcessor().GetWorkerCount())
{
    UASSERT(std::ranges::is_sorted(loading_component_names));
    UASSERT(std::ranges::adjacent_find(loading_component_names) == loading_component_names.end());

    components_.reserve(loading_component_names.size());

    if (manager.GetConfig().enable_component_load_tracing) {
        LOG_WARNING() << "Component loading tracing is enabled";
        auto& tp = engine::current_task::GetTaskProcessor();
        tp.RegisterPlugin(trace_plugin_);
    }

    for (auto& component_name : loading_component_names) {
        auto [_, success] = components_.emplace(std::move(component_name));
        UINVARIANT(success, "Duplicate component names");
    }

    if (!manager_.GetConfig().boot_log_path.empty()) {
        logging::LoggerConfig config;
        config.logger_name = "boot";
        config.file_path = manager_.GetConfig().boot_log_path;
        config.level = logging::Level::kTrace;
        config.format = logging::Format::kJson;
        config.truncate_on_start = true;
        config.queue_overflow_behavior = logging::QueueOverflowBehavior::kBlock;

        boot_logger_ = logging::impl::MakeTpLogger(config);
    }

    LOG_DEBUG() << "Starting component system";
    EmitBootEvent(Event::kComponentSystemIsStarting);

    StartPrintAddingComponentsTask();
}

RawComponentBase* ComponentContextImpl::AddComponent(
    std::string_view name,
    const ComponentConfig& config,
    const ComponentAdderBase& adder
) {
    LOG_DEBUG() << "Starting component " << name;
    EmitBootEvent(Event::kStarting, {{"component_name", name}});

    auto& component_info = GetComponentInfo(*this, name);
    const LoadingComponentScope loading_component_scope(*this, component_info);

    if (component_info.GetComponent()) {
        throw std::runtime_error("trying to add component " + std::string{name} + " multiple times");
    }

    // Put `context` on heap to detect use-after-free consistently.
    const auto context = std::make_unique<ComponentContext>(utils::impl::InternalTag{}, *this, component_info);

    component_info.SetComponent(adder.MakeComponent(config, *context));

    auto* component = component_info.GetComponent();
    if (component) {
        // Call the following command on logs to get the component dependencies:
        // sed -n 's/^.*component deps: \(.*\)$/\1/p'
        LOG_TRACE()
            << "component deps: "
            << fmt::format("\"{0}\" [label=\"{0}\n{1}\"]; ", name, compiler::GetTypeName(typeid(*component)))
            << component_info.GetDependencies();
    }

    LOG_DEBUG() << "Started component " << name;
    EmitBootEvent(Event::kStarted, {{"component_name", name}});

    return component;
}

void ComponentContextImpl::OnAllComponentsLoaded() {
    StopPrintAddingComponentsTask();

    service_lifetime_stage_ = ServiceLifetimeStage::kOnAllComponentsLoadedIsRunning;
    const tracing::Span span(kOnAllComponentsLoadedRootName);
    ProcessAllComponentLifetimeStageSwitchings(
        {impl::ComponentLifetimeStage::kRunning,
         [](ComponentInfo& component_info) { component_info.OnAllComponentsLoaded(); },
         "OnAllComponentsLoaded()",
         DependencyType::kNormal,
         true}
    );

    // In case no exception flies out, the service is fully loaded at this point.
    service_lifetime_stage_ = ServiceLifetimeStage::kRunning;

    LOG_INFO() << "All components loaded";
    EmitBootEvent(Event::kComponentSystemIsStarted);
}

void ComponentContextImpl::OnGracefulShutdownStarted() {
    const auto first_stage_interval = manager_.GetConfig().graceful_shutdown_continue_accepting_requests_interval;
    const auto second_stage_interval = manager_.GetConfig().graceful_shutdown_pending_requests_completion_interval;
    if (first_stage_interval <= std::chrono::milliseconds::zero() &&
        second_stage_interval <= std::chrono::milliseconds::zero())
    {
        return;
    }

    in_graceful_shutdown_.test_and_set();
    service_lifetime_stage_ = ServiceLifetimeStage::kGracefulShutdown;

    // First stage: new requests are still accepted, but health checks are failed
    LOG_INFO()
        << "Graceful shutdown: failing health handlers and continue accepting requests for " << first_stage_interval;
    if (first_stage_interval > std::chrono::milliseconds::zero()) {
        engine::SleepFor(first_stage_interval);
    }

    // Second stage: new requests are rejected, but already accepted requests are still being processed
    if (second_stage_interval <= std::chrono::milliseconds::zero()) {
        return;
    }
    const auto serving_shutdown_deadline = engine::Deadline::FromDuration(second_stage_interval);
    LOG_INFO()
        << "Graceful shutdown: closing all listeners and delaying active requests cancellation by "
        << second_stage_interval;
    ProcessAllComponentLifetimeStageSwitchings(
        {impl::ComponentLifetimeStage::kGracefulShutdown,
         [serving_shutdown_deadline](ComponentInfo& component_info) {
             component_info.OnGracefulShutdown(serving_shutdown_deadline);
         },
         "OnGracefulShutdown()",
         DependencyType::kNone,
         false}
    );
}

void ComponentContextImpl::OnAllComponentsAreStopping() {
    service_lifetime_stage_ = ServiceLifetimeStage::kOnAllComponentsAreStoppingIsRunning;
    LOG_INFO() << "Sending stopping notification to all components";
    ProcessAllComponentLifetimeStageSwitchings(
        {impl::ComponentLifetimeStage::kReadyForClearing,
         [](ComponentInfo& component_info) { component_info.OnAllComponentsAreStopping(); },
         "OnAllComponentsAreStopping()",
         DependencyType::kInverted,
         false}
    );
}

void ComponentContextImpl::ClearComponents() {
    if (components_.empty()) {
        // Already cleared
        return;
    }

    StopPrintAddingComponentsTask();
    const tracing::Span span(kClearComponentsRootName);
    OnAllComponentsAreStopping();

    service_lifetime_stage_ = ServiceLifetimeStage::kStopping;
    LOG_INFO() << "Stopping components";
    ProcessAllComponentLifetimeStageSwitchings(
        {impl::ComponentLifetimeStage::kNull,
         [](ComponentInfo& component_info) { component_info.ClearComponent(); },
         "ClearComponent()",
         DependencyType::kInverted,
         false}
    );

    components_.clear();
    LOG_INFO() << "Stopped all components";
}

engine::TaskProcessor& ComponentContextImpl::GetTaskProcessor(std::string_view name) const {
    return manager_.GetTaskProcessor(name);
}

const Manager& ComponentContextImpl::GetManager() const { return manager_; }

void ComponentContextImpl::CancelComponentsLoad() {
    CancelComponentLifetimeStageSwitching();
    if (components_load_cancelled_.test_and_set()) {
        return;
    }
    for (const auto& component_info : components_) {
        LOG_DEBUG() << "Call OnLoadingCancelled() for component '" << component_info->GetName() << "'";
        component_info->OnLoadingCancelled();
    }
}

bool ComponentContextImpl::IsAnyComponentInFatalState() const {
#ifndef NDEBUG
    {
        const auto data = shared_data_.Lock();
        UASSERT_MSG(
            data->print_adding_components_stopped,
            "IsAnyComponentInFatalState() should be called only after all "
            "the components has been loaded."
        );
    }
#endif

    for (const auto& comp : components_) {
        switch (comp->GetComponent()->GetComponentHealth()) {
            case ComponentHealth::kFatal:
                LOG_ERROR() << "Component '" << comp->GetName() << "' is in kFatal state";
                return true;
            case ComponentHealth::kFallback:
                LOG_LIMITED_WARNING() << "Component '" << comp->GetName() << "' is in kFallback state";
                break;
            case ComponentHealth::kOk:
                LOG_DEBUG() << "Component '" << comp->GetName() << "' is in kOk state";
                break;
        }
    }

    return false;
}

ServiceLifetimeStage ComponentContextImpl::GetServiceLifetimeStage() const { return service_lifetime_stage_.load(); }

bool ComponentContextImpl::IsInGracefulShutdown() const { return in_graceful_shutdown_.test(); }

bool ComponentContextImpl::HasDependencyOn(std::string_view component_name, std::string_view dependency) const {
    if (!Contains(component_name)) {
        UASSERT_MSG(
            false,
            fmt::format(
                "Exception while calling HasDependencyOn(\"{0}\", "
                "\"{1}\"). Component \"{0}\" was not loaded. Only "
                "dependency component is allowed to not be loaded.",
                component_name,
                dependency
            )
        );
        return false;
    }

    if (!Contains(dependency)) {
        return false;
    }

    if (component_name == dependency) {
        return false;
    }

    auto& from = GetComponentInfo(*this, dependency);
    auto& to = GetComponentInfo(*this, component_name);

    const auto data = shared_data_.Lock();
    UASSERT_MSG(
        data->print_adding_components_stopped,
        "HasDependencyOn() should be called only after all the "
        "components has been loaded."
    );

    std::unordered_set<ConstComponentInfoRef> handled;
    return FindDependencyPathDfs(from, to, handled, nullptr, *data);
}

std::unordered_set<std::string_view> ComponentContextImpl::GetAllDependencies(std::string_view component_name) const {
    UASSERT_MSG(
        Contains(component_name),
        fmt::format(
            "Exception while calling GetAllDependencies(\"{0}\" "
            "). Component \"{0}\" was not loaded.",
            component_name
        )
    );

    auto& from = GetComponentInfo(*this, component_name);

    const auto data = shared_data_.Lock();
    UASSERT_MSG(
        data->print_adding_components_stopped,
        "HasDependencyOn() should be called only after all the "
        "components has been loaded."
    );

    std::unordered_set<ConstComponentInfoRef> handled;
    FindAllDependenciesImpl(from, handled, *data);

    std::unordered_set<std::string_view> result;
    result.reserve(handled.size());
    for (auto value : handled) {
        if (&from != &*value) {
            result.insert(value->GetName());
        }
    }
    return result;
}

bool ComponentContextImpl::Contains(std::string_view name) const noexcept {
    return utils::ProjectedFind(components_, name) != components_.end();
}

void ComponentContextImpl::ThrowNonRegisteredComponent(
    std::string_view name,
    std::string_view type,
    ComponentInfo& current_component
) const {
    throw std::runtime_error(fmt::format(
        "Component '{}' requested component {} with name '{}'. That name is "
        "missing in the static config or the '{}' static config section contains "
        "'load-enabled: false'.",
        current_component.GetName(),
        type,
        name,
        name
    ));
}

void ComponentContextImpl::ThrowComponentTypeMismatch(
    std::string_view name,
    std::string_view type,
    RawComponentBase* component,
    ComponentInfo& current_component
) const {
    throw std::runtime_error(fmt::format(
        "Component '{}' requested component with name '{}' that is actually "
        "{}{} rather than a {}",
        current_component.GetName(),
        name,
        (component ? "has type " : "a nullptr"),
        (component ? compiler::GetTypeName(typeid(*component)) : std::string{}),
        type
    ));
}

void ComponentContextImpl::ProcessSingleComponentLifetimeStageSwitching(
    ComponentInfo& component_info,
    ComponentLifetimeStageSwitchingParams& params
) {
    const auto name = component_info.GetName();
    LOG_DEBUG() << "Preparing to call " << params.stage_switch_handler_name << " for component " << name;

    auto wait_cb = [&](ComponentInfo& other_component_info) {
        if (other_component_info.GetStage() != params.next_stage) {
            LOG_DEBUG(
                "Cannot call {} for component '{}' yet, waiting for component '{}' to complete its {} call.",
                params.stage_switch_handler_name,
                name,
                other_component_info.GetName(),
                params.stage_switch_handler_name
            );
            other_component_info.WaitStage(params.next_stage, params.stage_switch_handler_name);
        }
    };
    try {
        switch (params.dependency_type) {
            case DependencyType::kNormal:
                component_info.ForEachItDependsOn(wait_cb);
                break;
            case DependencyType::kInverted:
                component_info.ForEachDependsOnIt(wait_cb);
                break;
            case DependencyType::kNone:
                break;
        }

        LOG_DEBUG() << "Call " << params.stage_switch_handler_name << " for component '" << name << "'";
        EmitBootEvent(
            Event::kSwitching,
            {
                {"component_name", name},
                {"stage_switch_handler_name", params.stage_switch_handler_name},
            }
        );

        params.stage_switch_handler(component_info);
    } catch (const impl::StageSwitchingCancelledException& ex) {
        LOG_WARNING() << params.stage_switch_handler_name << " failed for component '" << name << "': " << ex;
        component_info.SetStage(params.next_stage);
        throw;
    } catch (const std::exception& ex) {
        LOG_ERROR() << params.stage_switch_handler_name << " failed for component '" << name << "': " << ex;
        if (params.allow_cancelling) {
            component_info.SetStageSwitchingCancelled(true);
            if (!params.is_component_lifetime_stage_switchings_cancelled.exchange(true)) {
                CancelComponentLifetimeStageSwitching();
            }
            component_info.SetStage(params.next_stage);
            throw;
        }
    }
    component_info.SetStage(params.next_stage);
}

void ComponentContextImpl::ProcessAllComponentLifetimeStageSwitchings(ComponentLifetimeStageSwitchingParams params) {
    PrepareComponentLifetimeStageSwitching();

    std::vector<std::pair<ComponentInfoRef, engine::TaskWithResult<void>>> tasks;
    tasks.reserve(components_.size());
    for (auto& component_info : components_) {
        tasks.emplace_back(*component_info, utils::TaskBuilder{}.NoSpan().Background().Critical().Build([&] {
            ProcessSingleComponentLifetimeStageSwitching(*component_info, params);
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
        if (params.allow_cancelling && !params.is_component_lifetime_stage_switchings_cancelled.exchange(true)) {
            CancelComponentLifetimeStageSwitching();
        }

        for (auto& task_item : tasks) {
            if (task_item.second.IsValid()) {
                task_item.second.Wait();
            }
        }

        throw;
    }

    if (params.is_component_lifetime_stage_switchings_cancelled) {
        throw std::logic_error(
            params.stage_switch_handler_name + " cancelled but only StageSwitchingCancelledExceptions were caught"
        );
    }
}

RawComponentBase* ComponentContextImpl::DoFindComponent(std::string_view name, ComponentInfo& current_component) {
    auto& component_info = GetComponentInfo(*this, name);

    AddDependency(component_info, current_component);

    if (auto* const component = component_info.GetComponent()) {
        return component;
    }

    LOG_DEBUG()
        << "Component '" << name << "' is not loaded yet, component '" << current_component.GetName()
        << "' is waiting for it to load";
    EmitBootEvent(
        Event::kWaitingForComponent,
        {
            {"component_name", current_component.GetName()},
            {"dependency_name", name},
        }
    );

    const SearchingComponentScope finder(*this, current_component);

    return component_info.WaitAndGetComponent();
}

void ComponentContextImpl::AddDependency(ComponentInfo& dependency, ComponentInfo& current_component) {
    auto data = shared_data_.Lock();

    if (current_component.CheckItDependsOn(dependency)) {
        return;
    }

    LOG_DEBUG() << "Resolving dependency " << current_component.GetName() << " -> " << dependency.GetName();
    EmitBootEvent(
        Event::kFindComponent,
        {
            {"component_name", current_component.GetName()},
            {"dependency_name", dependency.GetName()},
        }
    );

    CheckForDependencyCycle(current_component, dependency, *data);

    current_component.AddItDependsOn(dependency);
    dependency.AddDependsOnIt(current_component);
}

bool ComponentContextImpl::FindDependencyPathDfs(
    const ComponentInfo& current,
    const ComponentInfo& target,
    std::unordered_set<ConstComponentInfoRef>& handled,
    std::vector<ConstComponentInfoRef>* dependency_path,
    const ProtectedData& data
) const {
    handled.insert(current);
    bool found = (&current == &target);

    current.ForEachDependsOnIt([&](ComponentInfo& dependent) {
        if (!found && !handled.count(dependent)) {
            found = FindDependencyPathDfs(dependent, target, handled, dependency_path, data);
        }
    });

    if (found && dependency_path) {
        dependency_path->push_back(current);
    }

    return found;
}

void ComponentContextImpl::FindAllDependenciesImpl(
    const ComponentInfo& current,
    std::unordered_set<ConstComponentInfoRef>& handled,
    const ProtectedData& data
) const {
    handled.insert(current);

    current.ForEachItDependsOn([&](ComponentInfo& dependency) {
        if (!handled.count(dependency)) {
            FindAllDependenciesImpl(dependency, handled, data);
        }
    });
}

void ComponentContextImpl::CheckForDependencyCycle(
    const ComponentInfo& new_dependency_from,
    const ComponentInfo& new_dependency_to,
    const ProtectedData& data
) const {
    std::unordered_set<ConstComponentInfoRef> handled;
    std::vector<ConstComponentInfoRef> dependency_chain;

    if (FindDependencyPathDfs(new_dependency_from, new_dependency_to, handled, &dependency_chain, data)) {
        dependency_chain.push_back(new_dependency_to);
        auto msg = JoinNamesFromInfo(dependency_chain, " -> ");
        LOG_ERROR() << "Found circular dependency between components: " << msg;
        throw std::runtime_error("circular components dependency: " + msg);
    }
}

void ComponentContextImpl::PrepareComponentLifetimeStageSwitching() {
    for (auto& component_item : components_) {
        component_item->SetStageSwitchingCancelled(false);
    }
}

void ComponentContextImpl::CancelComponentLifetimeStageSwitching() {
    EmitBootEvent(Event::kLoadingIsCancelled);
    for (auto& component_item : components_) {
        component_item->SetStageSwitchingCancelled(true);
    }
}

void ComponentContextImpl::StartPrintAddingComponentsTask() {
    print_adding_components_task_ = utils::TaskBuilder{}.NoSpan().Background().Critical().Build([this]() {
        for (;;) {
            {
                auto data = shared_data_.UniqueLock();
                print_adding_components_cv_
                    .WaitFor(data.GetLock(), manager_.GetConfig().component_load_print_interval, [&data]() {
                        return data->print_adding_components_stopped;
                    });
                if (data->print_adding_components_stopped) {
                    return;
                }
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

    auto& tp = engine::current_task::GetTaskProcessor();
    tp.UnregisterPlugin(trace_plugin_);
    trace_plugin_.Clear();
}

void ComponentContextImpl::PrintAddingComponents() const {
    std::vector<ConstComponentInfoRef> adding_components;
    std::vector<ConstComponentInfoRef> busy_components;

    {
        auto data = shared_data_.Lock();
        adding_components.reserve(data->loading_components.size());
        for (const auto& component : data->loading_components) {
            adding_components.push_back(component);
            if (data->searching_components.count(component) == 0) {
                busy_components.push_back(component);
            }
        }
    }
    LOG_INFO()
        << "still adding components, busy: [" << JoinNamesFromInfo(busy_components, ", ") << "], loading: ["
        << JoinNamesFromInfo(adding_components, ", ") << ']';

    trace_plugin_.PrintStacksByComponentNames(ExtractNamesFromInfo(busy_components));
}

std::string_view ComponentContextImpl::ToString(Event event)
{
    switch (event) {
        case Event::kComponentSystemIsStarting:
            return "component_system_is_starting";
        case Event::kStarting:
            return "component_is_starting";
        case Event::kStarted:
            return "component_is_created";
        case Event::kComponentSystemIsStarted:
            return "component_system_is_started";
        case Event::kLoadingIsCancelled:
            return "loading_is_cancelled";
        case Event::kSwitching:
            return "component_is_switching_stage";
        case Event::kWaitingForComponent:
            return "waiting_for_component";
        case Event::kFindComponent:
            return "find_component";
    }
    // suppress error on gcc-10 and gcc-11
    throw std::runtime_error("Unexpected event");
}

void ComponentContextImpl::EmitBootEvent(Event event, logging::LogExtra log_extra) const {
    if (!boot_logger_) {
        return;
    }

    log_extra.Extend("event_type", std::string{ToString(event)});
    LOG_INFO_TO(*boot_logger_) << log_extra;

    if (event == Event::kComponentSystemIsStarted) {
        boot_logger_->Flush();
    }
}

}  // namespace components::impl

USERVER_NAMESPACE_END
