#pragma once

#include <userver/components/component_context.hpp>

#include <atomic>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <userver/concurrent/variable.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>

#include <components/component_context_component_info.hpp>
#include <components/impl/component_name_from_info.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

struct ComponentContext::Impl {
  Impl(const Manager& manager,
       std::vector<std::string>&& loading_component_names);

  impl::ComponentBase* AddComponent(std::string_view name,
                                    const ComponentFactory& factory,
                                    ComponentContext& context);

  void OnAllComponentsLoaded();

  void OnAllComponentsAreStopping();

  void ClearComponents();

  engine::TaskProcessor& GetTaskProcessor(const std::string& name) const;

  const Manager& GetManager() const;

  void CancelComponentsLoad();

  bool IsAnyComponentInFatalState() const;

  bool Contains(std::string_view name) const noexcept;

  [[noreturn]] void ThrowNonRegisteredComponent(std::string_view name,
                                                std::string_view type) const;
  [[noreturn]] void ThrowComponentTypeMismatch(
      std::string_view name, std::string_view type,
      impl::ComponentBase* component) const;

  impl::ComponentBase* DoFindComponent(std::string_view name);

 private:
  class TaskToComponentMapScope final {
   public:
    TaskToComponentMapScope(Impl& context,
                            impl::ComponentNameFromInfo component_name);
    ~TaskToComponentMapScope();

   private:
    Impl& context_;
  };

  class SearchingComponentScope final {
   public:
    SearchingComponentScope(const Impl& context,
                            impl::ComponentNameFromInfo component_name);
    ~SearchingComponentScope();

   private:
    const Impl& context_;
    impl::ComponentNameFromInfo component_name_;
  };

  using ComponentMap =
      std::unordered_map<impl::ComponentNameFromInfo, impl::ComponentInfo>;

  enum class DependencyType { kNormal, kInverted };

  struct ProtectedData {
    std::unordered_map<engine::impl::TaskContext*, impl::ComponentNameFromInfo>
        task_to_component_map;
    mutable std::unordered_set<impl::ComponentNameFromInfo>
        searching_components;
    bool print_adding_components_stopped{false};
  };

  struct ComponentLifetimeStageSwitchingParams {
    ComponentLifetimeStageSwitchingParams(
        const impl::ComponentLifetimeStage& next_stage,
        void (impl::ComponentInfo::*stage_switch_handler)(),
        const std::string& stage_switch_handler_name,
        DependencyType dependency_type, bool allow_cancelling)
        : next_stage(next_stage),
          stage_switch_handler(stage_switch_handler),
          stage_switch_handler_name(stage_switch_handler_name),
          dependency_type(dependency_type),
          allow_cancelling(allow_cancelling),
          is_component_lifetime_stage_switchings_cancelled{false} {}

    const impl::ComponentLifetimeStage& next_stage;
    void (impl::ComponentInfo::*stage_switch_handler)();
    const std::string& stage_switch_handler_name;
    DependencyType dependency_type;
    bool allow_cancelling;
    std::atomic<bool> is_component_lifetime_stage_switchings_cancelled;
  };

  void ProcessSingleComponentLifetimeStageSwitching(
      impl::ComponentNameFromInfo name, impl::ComponentInfo& component_info,
      ComponentLifetimeStageSwitchingParams& params);

  void ProcessAllComponentLifetimeStageSwitchings(
      ComponentLifetimeStageSwitchingParams params);

  void AddDependency(impl::ComponentNameFromInfo name);

  bool FindDependencyPathDfs(
      impl::ComponentNameFromInfo current, impl::ComponentNameFromInfo target,
      std::set<impl::ComponentNameFromInfo>& handled,
      std::vector<impl::ComponentNameFromInfo>& dependency_path,
      const ProtectedData& data) const;
  void CheckForDependencyCycle(impl::ComponentNameFromInfo new_dependency_from,
                               impl::ComponentNameFromInfo new_dependency_to,
                               const ProtectedData& data) const;

  void PrepareComponentLifetimeStageSwitching();
  void CancelComponentLifetimeStageSwitching();

  static impl::ComponentNameFromInfo GetLoadingComponentName(
      const ProtectedData&);

  void StartPrintAddingComponentsTask();
  void StopPrintAddingComponentsTask();
  void PrintAddingComponents() const;

  const Manager& manager_;

  ComponentMap components_;
  std::atomic_flag components_load_cancelled_ ATOMIC_FLAG_INIT;

  engine::ConditionVariable print_adding_components_cv_;
  concurrent::Variable<ProtectedData> shared_data_;
  engine::TaskWithResult<void> print_adding_components_task_;
};

}  // namespace components

USERVER_NAMESPACE_END
