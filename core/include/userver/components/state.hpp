#pragma once

/// @file userver/components/state.hpp
/// @brief @copybrief components::State

#include <string>
#include <string_view>
#include <unordered_set>

USERVER_NAMESPACE_BEGIN

namespace components {

class ComponentContext;

namespace impl {
class ComponentContextImpl;
}

/// State of components that is usable after the components constructor and
/// until all the components were not destroyed.
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
  /// Components construction should finish before any call to this function
  /// is made.
  ///
  /// Note that GetAllDependencies usually is more effective, if you are
  /// planning multiple calls for the same component name.
  bool HasDependencyOn(std::string_view component_name,
                       std::string_view dependency) const;

  /// @returns all the components that `component_name` depends on directly or
  /// transitively.
  ///
  /// Component with name `component_name` should be loaded.
  /// Components construction should finish before any call to this function
  /// is made. The result should now outlive the all the components
  /// destruction.
  std::unordered_set<std::string_view> GetAllDependencies(
      std::string_view component_name) const;

 private:
  const impl::ComponentContextImpl& impl_;
};

}  // namespace components

USERVER_NAMESPACE_END
