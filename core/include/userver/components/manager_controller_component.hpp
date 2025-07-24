#pragma once

/// @file userver/components/manager_controller_component.hpp
/// @brief @copybrief components::ManagerControllerComponent

#include <userver/components/component_fwd.hpp>
#include <userver/components/raw_component_base.hpp>
#include <userver/concurrent/async_event_source.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace impl {
class Manager;
}  // namespace impl

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that prepares the engine internals and starts all the
/// other components.
///
/// ## ManagerControllerComponent Dynamic config
/// * @ref USERVER_TASK_PROCESSOR_PROFILER_DEBUG
/// * @ref USERVER_TASK_PROCESSOR_QOS
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// coro_pool.initial_size | amount of coroutines to preallocate on startup | 1000
/// coro_pool.max_size | max amount of coroutines to keep preallocated | 4000
/// coro_pool.stack_size | size of a single coroutine stack @ref scripts/docs/en/userver/stack.md | 256 * 1024
/// coro_pool.local_cache_size | local coroutine cache size per thread | 32
/// coro_pool.stack_usage_monitor_enabled | whether stack usage is accounted and warnings about too high stack usage are logged @ref scripts/docs/en/userver/stack.md | true
/// event_thread_pool.threads | number of threads to process low level IO system calls (number of ev loops to start in libev) | 2
/// event_thread_pool.thread_name | set OS thread name to this value | 'event-worker'
/// components | dictionary of "component name": "options" | -
/// default_task_processor | name of the default task processor to use in components | main-task-processor
/// fs_task_processor | name of the blocking task processor to use in components | fs-task-processor
/// task_processors.*NAME*.*OPTIONS* | dictionary of task processors to create and their options. See description below | -
/// mlock_debug_info | whether to mlock(2) process debug info to prevent major page faults on unwinding | true
/// disable_phdr_cache | whether to disable caching of phdr_info objects. Usable if rebuilding with cmake variable USERVER_DISABLE_PHDR_CACHE is off limits, and has the same effect | false
/// static_config_validation.validate_all_components | whether to validate static config according to schema; should be `true` for all new services | true
/// preheat_stacktrace_collector | whether to collect a dummy stacktrace at server start up (usable to avoid loading debug info at random point at runtime) | true
/// userver_experiments.*NAME* | whether to enable certain userver experiments; these are gradually enabled by userver team, for internal use only | false
/// graceful_shutdown_interval | at shutdown, first hang for this duration with /ping 5xx to give the balancer a chance to redirect new requests to other hosts | 0s
///
/// ## Static task_processor options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// guess-cpu-limit | guess optimal threads count | false
/// thread_name | set OS thread name to this value | Part of the task_processor name before the first '-' symbol with '-worker' appended; for example 'fs-worker' or 'main-worker'
/// worker_threads | threads count for the task processor | -
/// os-scheduling | OS scheduling mode for the task processor threads. 'idle' sets the lowest priority. 'low-priority' sets the priority below 'normal' but higher than 'idle'. | normal
/// spinning-iterations | tunes the number of spin-wait iterations in case of an empty task queue before threads go to sleep | 1000
/// task-processor-queue | Task queue mode for the task processor. `global-task-queue` default task queue. `work-stealing-task-queue` experimental with potentially better scalability than `global-task-queue`. | global-task-queue
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
class ManagerControllerComponent final : public RawComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of components::ManagerControllerComponent
    static constexpr std::string_view kName = "manager-controller";

    ManagerControllerComponent(const components::ComponentConfig& config, const components::ComponentContext& context);

    ~ManagerControllerComponent() override;

private:
    void WriteStatistics(utils::statistics::Writer& writer);

    void OnConfigUpdate(const dynamic_config::Snapshot& cfg);

    const components::impl::Manager& components_manager_;
    utils::statistics::Entry statistics_holder_;
    concurrent::AsyncEventSubscriberScope config_subscription_;
};

template <>
inline constexpr auto kConfigFileMode<ManagerControllerComponent> = ConfigFileMode::kNotRequired;

}  // namespace components

USERVER_NAMESPACE_END
