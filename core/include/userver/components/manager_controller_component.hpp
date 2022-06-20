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
/// coro_pool.initial_size | amount of coroutines to preallocate on startup | -
/// coro_pool.max_size | max amount of coroutines to keep preallocated | -
/// coro_pool.stack_size | size of a single coroutine | 256 * 1024
/// event_thread_pool.threads | number of threads to process low level IO system calls (number of ev loops to start in libev) | -
/// components | dictionary of "component name": "options" | -
/// task_processors | dictionary of task processors to create and their options | -
/// task_processors.*NAME*.thread_name | set OS thread name to this value | -
/// task_processors.*NAME*.worker_threads | threads count for the task processor | -
/// default_task_processor | name of the default task processor to use in components | -
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

  static constexpr std::string_view kName = "manager-controller";

 private:
  formats::json::Value ExtendStatistics(
      const utils::statistics::StatisticsRequest& /*request*/);

  void OnConfigUpdate(const dynamic_config::Snapshot& cfg);

 private:
  const components::Manager& components_manager_;
  utils::statistics::Entry statistics_holder_;
  concurrent::AsyncEventSubscriberScope config_subscription_;
};

template <>
inline constexpr auto kConfigFileMode<ManagerControllerComponent> =
    ConfigFileMode::kNotRequired;

}  // namespace components

USERVER_NAMESPACE_END
