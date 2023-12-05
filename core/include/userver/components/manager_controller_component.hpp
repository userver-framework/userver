#pragma once

/// @file userver/components/manager_controller_component.hpp
/// @brief @copybrief components::ManagerControllerComponent

#include <userver/components/component_fwd.hpp>
#include <userver/components/impl/component_base.hpp>
#include <userver/concurrent/async_event_source.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

class Manager;

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that prepares the engine internals and starts all the
/// other components.
///
/// ## Dynamic config
/// * @ref USERVER_TASK_PROCESSOR_PROFILER_DEBUG
/// * @ref USERVER_TASK_PROCESSOR_QOS
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// coro_pool.initial_size | amount of coroutines to preallocate on startup | 1000
/// coro_pool.max_size | max amount of coroutines to keep preallocated | 4000
/// coro_pool.stack_size | size of a single coroutine | 256 * 1024
/// event_thread_pool.threads | number of threads to process low level IO system calls (number of ev loops to start in libev) | 2
/// event_thread_pool.thread_name | set OS thread name to this value | 'event-worker'
/// components | dictionary of "component name": "options" | -
/// default_task_processor | name of the default task processor to use in components | -
/// task_processors.*NAME*.*OPTIONS* | dictionary of task processors to create and their options. See description below | -
/// mlock_debug_info | whether to mlock(2) process debug info to prevent major page faults on unwinding | true
/// disable_phdr_cache | whether to disable caching of phdr_info objects. Usable if rebuilding with cmake variable USERVER_DISABLE_PHDR_CACHE is off limits, and has the same effect | false
///
/// ## Static task_processor options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// guess-cpu-limit | guess optimal threads count | false
/// thread_name | set OS thread name to this value | Part of the task_processor name before the first '-' symbol with '-worker' appended; for example 'fs-worker' or 'main-worker'
/// worker_threads | threads count for the task processor | -
/// os-scheduling | OS scheduling mode for the task processor threads. 'idle' sets the lowest priority. 'low-priority' sets the priority below 'normal' but higher than 'idle'. | normal
/// spinning-iterations | tunes the number of spin-wait iterations in case of an empty task queue before threads go to sleep | 10000
/// task-trace | optional dictionary of tracing options | empty (disabled)
/// task-trace.every | set N to trace each Nth task | 1000
/// task-trace.max-context-switch-count | set upper limit of context switches to trace for a single task | 1000
/// task-trace.logger | required name of logger to write traces to, should not be the 'default' logger | -
///
/// Tips and tricks on `task-trace` usage are described in
/// @ref scripts/docs/en/userver/profile_context_switches.md.
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample components manager config component config

// clang-format on
class ManagerControllerComponent final : public impl::ComponentBase {
 public:
  ManagerControllerComponent(const components::ComponentConfig& config,
                             const components::ComponentContext& context);

  ~ManagerControllerComponent() override;

  /// @ingroup userver_component_names
  /// @brief The default name of components::ManagerControllerComponent
  static constexpr std::string_view kName = "manager-controller";

 private:
  void WriteStatistics(utils::statistics::Writer& writer);

  void OnConfigUpdate(const dynamic_config::Snapshot& cfg);

  const components::Manager& components_manager_;
  utils::statistics::Entry statistics_holder_;
  concurrent::AsyncEventSubscriberScope config_subscription_;
};

template <>
inline constexpr auto kConfigFileMode<ManagerControllerComponent> =
    ConfigFileMode::kNotRequired;

}  // namespace components

USERVER_NAMESPACE_END
