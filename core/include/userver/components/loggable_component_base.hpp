#pragma once

/// @file userver/components/loggable_component_base.hpp
/// @brief Contains components::LoggableComponentBase declaration and forward
/// declarations of components::ComponentConfig and
/// components::ComponentContext, function components::GetCurrentComponentName()

#include <userver/components/component_fwd.hpp>
#include <userver/components/impl/component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

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

  /// Disgusting crutch added to prevent the need to specify server handlers
  /// twice in the service config.
  /// Don't use it.
  void OnAllComponentsLoaded() override {}

  /// Same as OnAllComponentsLoaded(). Just do not use it.
  void OnAllComponentsAreStopping() override {}
};

}  // namespace components

USERVER_NAMESPACE_END
