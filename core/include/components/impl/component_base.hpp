#pragma once

namespace components::impl {

/// Don't use it for application components, use LoggableComponentBase instead
class ComponentBase {
 public:
  /// There is no action in a base constructor but you have to initialize
  /// a real component in its constructor.
  /// Also you have to call `component_context.FindComponent<>()` here for all
  /// dependant components and, if necessary, store returned references for
  /// future use.
  /// You may not call FindComponent() outside of the constructor.
  /// It is guaranteed that objects these references point to will not be
  /// destroyed for the total lifetime of the current component.
  ComponentBase() = default;

  ComponentBase(ComponentBase&&) = delete;

  ComponentBase(const ComponentBase&) = delete;

  /// It is a good place to stop your work here.
  /// All components dependent on the current component were destroyed.
  /// All components on which the current component depends are still alive.
  virtual ~ComponentBase() = default;

  /// Called once if the creation of any other component failed.
  /// If the current component expects some other component to take any action
  /// with the current component, this call is a signal that such action may
  /// never happen due to components loading was cancelled.
  /// Application components might not want to override it.
  virtual void OnLoadingCancelled() {}

  /// Disgusting crutch added to prevent the need to specify server handlers
  /// twice in the service config.
  /// Don't use it.
  virtual void OnAllComponentsLoaded() {}

  /// Same as OnAllComponentsLoaded(). Just do not use it.
  virtual void OnAllComponentsAreStopping() {}
};

}  // namespace components::impl
