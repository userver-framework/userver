#pragma once

/// @file userver/components/state.hpp
/// @brief @copybrief components::State

#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace components {

class ComponentContext;

namespace impl {
class ComponentContextImpl;
}

/// State of components that is usable after the components constructor.
///
/// @see components::ComponentContext
class State final {
 public:
  explicit State(const ComponentContext& cc) noexcept;

  /// @returns true if one of the components is in fatal state and can not
  /// work. A component is in fatal state if the
  /// components::ComponentHealth::kFatal value is returned from the overridden
  /// components::LoggableComponentBase::GetComponentHealth().
  bool IsAnyComponentInFatalState() const;

  /// @returns true if component with name `component_name` depends
  /// (directly or transitively) on a component with name `dependency`.
  ///
  /// Component with name `component_name` should be loaded.
  bool HasDependencyOn(std::string_view component_name,
                       std::string_view dependency) const;

 private:
  const impl::ComponentContextImpl& impl_;
};

}  // namespace components

USERVER_NAMESPACE_END
