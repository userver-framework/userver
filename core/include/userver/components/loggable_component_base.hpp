#pragma once

/// @file userver/components/loggable_component_base.hpp
/// @brief Contains components::LoggableComponentBase declaration and forward
/// declarations of components::ComponentConfig and
/// components::ComponentContext, function components::GetCurrentComponentName()

#include <userver/components/component_fwd.hpp>
#include <userver/components/impl/component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_base_classes
///
/// @brief Base class for all @ref userver_components "application components",
/// it depends on components::Logger and components::Tracer.
class LoggableComponentBase : public impl::ComponentBase {
 public:
  LoggableComponentBase(const ComponentConfig&, const ComponentContext&);

  LoggableComponentBase(LoggableComponentBase&&) = delete;
  LoggableComponentBase(const LoggableComponentBase&) = delete;

  /// It is a good place to stop your work here.
  /// All components dependent on the current component were destroyed.
  /// All components on which the current component depends are still alive.
  ~LoggableComponentBase() override = default;

  /// Override this function to inform the world of the state of your
  /// component.
  ///
  /// @warning The function is called concurrently from multiple threads.
  ComponentHealth GetComponentHealth() const override {
    return ComponentHealth::kOk;
  }

  /// Called once if the creation of any other component failed.
  /// If the current component expects some other component to take any action
  /// with the current component, this call is a signal that such action may
  /// never happen due to components loading was cancelled.
  /// Application components might not want to override it.
  void OnLoadingCancelled() override {}

  /// Component may use this function to finalize registration of other
  /// components that depend on it (for example, handler components register
  /// in server component, and the latter uses OnAllComponentsLoaded() to start
  /// processing requests).
  ///
  /// Base components may override it and make `final` to do some work after the
  /// derived object constructor is called. Don't use it otherwise.
  void OnAllComponentsLoaded() override {}

  /// Component may use this function to stop doing work before the stop of the
  /// components that depend on it.
  ///
  /// Base components may override it and make `final` to do some work before
  /// the derived object constructor is called. Don't use it otherwise.
  void OnAllComponentsAreStopping() override {}

  static yaml_config::Schema GetStaticConfigSchema();
};

}  // namespace components

USERVER_NAMESPACE_END
