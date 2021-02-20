#pragma once

/// @file components/manager_controller_component.hpp
/// @brief @copybrief components::ManagerControllerComponent

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/impl/component_base.hpp>
#include <engine/task/task_processor_fwd.hpp>
#include <utils/statistics/storage.hpp>

#include <taxi_config/storage/component.hpp>

namespace components {

class Manager;

/// @defgroup userver_components Userver Components
///
/// @brief Components that could be added to components::ComponentList for
/// further use with utils::DaemonMain, components::Run or components::RunOnce.
///
/// Any userver based service consists of components. Component is a basic
/// building block that encapsulates logic with configuration and is able to
/// interact with other components.
///
/// ## Components static configuration
/// components::ManagerControllerComponent starts all the components that
/// were added to the components::ComponentList. Each registered component
/// should have a section in service config (also known as static config).
///
/// The component configuration is passed as a first parameter of type
/// components::ComponentConfig to the constructor of the component. Note that
/// components::ComponentConfig extends the functionality of
/// yaml_config::YamlConfig that is able to substitute variables with values,
/// use fallbacks. See yaml_config::YamlConfig for more info and examples.
///
/// ## Startup context
/// On component construction a components::ComponentContext is passed as a
/// second parameter to the constructor of the component. That context could
/// be used to get references to other components. That reference to the
/// component is guaranteed to outlive the component that is being constructed.
///
/// ## Components construction order
/// utils::DaemonMain, components::Run or components::RunOnce
/// start all the components from the passed components::ComponentList.
/// Each component is constructed in a separate engine::Task which makes them
/// initialize concurrently. This is a useful feature, for examples in cases
/// with multiple caches that slowly read from different databases.
///
/// To make component *A* depend on component *B* just call
/// components::ComponentContext::FindComponent<B>() in the constructor of A.
/// FindComponent() suspends the current task and continues only after the
/// construction of component B is finished. Components are destroyed
/// in reverse order of construction, so the component A is destroyed before
/// the component B. In other words - references from FindComponent() outlive
/// the component that called the FindComponent() function. If any component
/// loading fails, FindComponent() wakes up and throws an
/// components::ComponentsLoadCancelledException.
///
/// ## References from components
/// It is a common practice to have a component that returns reference *R* from
/// some function *F*. In such cases:
/// * a reference *R* lives as long as the component is alive
/// * and it should be safe to invoke member function of reference *R*
/// concurrently with member function invocation on a result of other F
/// invocation.
///
/// Examples:
/// * components::HttpClient::GetHttpClient()
/// * components::StatisticsStorage::GetStorage()

// clang-format off

/// @ingroup userver_components
///
/// @brief Component to start all the other components
///
/// The component must be configured in service config.
///
/// ## Available options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// coro_pool.initial_size | amount of coroutines to preallocate on startup | -
/// coro_pool.max_size | max amount of coroutines to keep preallocated | -
/// event_thread_pool.threads | number of threads to process low level IO system calls (number of ev loops to start in libev) | -
/// components | dictionary of "component name": "options" | -
/// task_processors | dictionary of task processors and their options | -
/// task_processors.*NAME*.thread_name | set OS thread name to this value | -
/// task_processors.*NAME*.worker_threads | threads count for the task processor | -
/// default_task_processor | name of the default task processor to use in components | -
///
/// ## Configuration example:
///
/// @snippet components/common_component_list_test.cpp  Sample components manager config component config

// clang-format on
class ManagerControllerComponent final : public impl::ComponentBase {
 public:
  ManagerControllerComponent(const components::ComponentConfig& config,
                             const components::ComponentContext& context);

  ~ManagerControllerComponent() override;

  static constexpr const char* kName = "manager-controller";

 private:
  formats::json::Value ExtendStatistics(
      const utils::statistics::StatisticsRequest& /*request*/);

  using TaxiConfigPtr = std::shared_ptr<const taxi_config::Config>;
  void OnConfigUpdate(const TaxiConfigPtr& cfg);

 private:
  const components::Manager& components_manager_;
  utils::statistics::Entry statistics_holder_;
  utils::AsyncEventSubscriberScope config_subscription_;
};

}  // namespace components
