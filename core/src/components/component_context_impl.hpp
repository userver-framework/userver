#pragma once

#include <atomic>
#include <unordered_set>
#include <vector>

#include <userver/components/state.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/projected_set.hpp>

#include <components/component_context_component_info.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {
class ComponentConfig;
class ComponentContext;
}  // namespace components

namespace components::impl {

class Manager;
enum class ComponentLifetimeStage;
class ComponentInfo;
class ComponentAdderBase;

std::string_view ComponentInfoProjection(const utils::impl::MutableWrapper<ComponentInfo>& component_info);

// Responsible for controlling lifetime stages and dependencies of components.
class ComponentContextImpl {
public:
    ComponentContextImpl(const Manager& manager, std::vector<std::string>&& loading_component_names);

    RawComponentBase*
    AddComponent(std::string_view name, const ComponentConfig& config, const ComponentAdderBase& adder);

    void OnAllComponentsLoaded();

    void OnGracefulShutdownStarted();

    void OnAllComponentsAreStopping();

    void ClearComponents();

    engine::TaskProcessor& GetTaskProcessor(std::string_view name) const;

    const Manager& GetManager() const;

    void CancelComponentsLoad();

    bool IsAnyComponentInFatalState() const;

    ServiceLifetimeStage GetServiceLifetimeStage() const;

    bool HasDependencyOn(std::string_view component_name, std::string_view dependency) const;

    std::unordered_set<std::string_view> GetAllDependencies(std::string_view component_name) const;

    bool Contains(std::string_view name) const noexcept;

    [[noreturn]] void
    ThrowNonRegisteredComponent(std::string_view name, std::string_view type, ComponentInfo& current_component) const;

    [[noreturn]] void ThrowComponentTypeMismatch(
        std::string_view name,
        std::string_view type,
        RawComponentBase* component,
        ComponentInfo& current_component
    ) const;

    RawComponentBase* DoFindComponent(std::string_view name, ComponentInfo& current_component);

private:
    class LoadingComponentScope final {
    public:
        LoadingComponentScope(ComponentContextImpl& context, ComponentInfo& component);
        ~LoadingComponentScope();

    private:
        ComponentContextImpl& context_;
        ComponentInfo& component_;
    };

    class SearchingComponentScope final {
    public:
        SearchingComponentScope(const ComponentContextImpl& context, ComponentInfo& component);
        ~SearchingComponentScope();

    private:
        const ComponentContextImpl& context_;
        ComponentInfo& component_;
    };

    using ComponentMap =
        utils::ProjectedUnorderedSet<utils::impl::MutableWrapper<ComponentInfo>, &ComponentInfoProjection>;

    enum class DependencyType { kNormal, kInverted };

    struct ProtectedData {
        std::unordered_set<ComponentInfoRef> loading_components;
        mutable std::unordered_set<ComponentInfoRef> searching_components;
        bool print_adding_components_stopped{false};
    };

    struct ComponentLifetimeStageSwitchingParams {
        ComponentLifetimeStageSwitchingParams(
            const ComponentLifetimeStage& next_stage,
            void (ComponentInfo::*stage_switch_handler)(),
            const std::string& stage_switch_handler_name,
            DependencyType dependency_type,
            bool allow_cancelling
        )
            : next_stage(next_stage),
              stage_switch_handler(stage_switch_handler),
              stage_switch_handler_name(stage_switch_handler_name),
              dependency_type(dependency_type),
              allow_cancelling(allow_cancelling),
              is_component_lifetime_stage_switchings_cancelled{false} {}

        const ComponentLifetimeStage& next_stage;
        void (ComponentInfo::*stage_switch_handler)();
        const std::string& stage_switch_handler_name;
        DependencyType dependency_type;
        bool allow_cancelling;
        std::atomic<bool> is_component_lifetime_stage_switchings_cancelled;
    };

    void ProcessSingleComponentLifetimeStageSwitching(
        ComponentInfo& component_info,
        ComponentLifetimeStageSwitchingParams& params
    );

    void ProcessAllComponentLifetimeStageSwitchings(ComponentLifetimeStageSwitchingParams params);

    void AddDependency(ComponentInfo& dependency, ComponentInfo& current_component);

    bool FindDependencyPathDfs(
        const ComponentInfo& current,
        const ComponentInfo& target,
        std::unordered_set<ConstComponentInfoRef>& handled,
        std::vector<ConstComponentInfoRef>* dependency_path,
        const ProtectedData& data
    ) const;

    void FindAllDependenciesImpl(
        const ComponentInfo& current,
        std::unordered_set<ConstComponentInfoRef>& handled,
        const ProtectedData& data
    ) const;

    void CheckForDependencyCycle(
        const ComponentInfo& new_dependency_from,
        const ComponentInfo& new_dependency_to,
        const ProtectedData& data
    ) const;

    void PrepareComponentLifetimeStageSwitching();
    void CancelComponentLifetimeStageSwitching();

    template <typename Self>
    static auto& GetComponentInfo(Self& self, std::string_view component_name);

    void StartPrintAddingComponentsTask();
    void StopPrintAddingComponentsTask();
    void PrintAddingComponents() const;

    const Manager& manager_;

    ComponentMap components_;
    std::atomic_flag components_load_cancelled_ ATOMIC_FLAG_INIT;
    std::atomic<ServiceLifetimeStage> service_lifetime_stage_{ServiceLifetimeStage::kLoading};

    engine::ConditionVariable print_adding_components_cv_;
    concurrent::Variable<ProtectedData> shared_data_;
    engine::TaskWithResult<void> print_adding_components_task_;
};

}  // namespace components::impl

USERVER_NAMESPACE_END
