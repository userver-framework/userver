#pragma once

/// @file userver/components/state.hpp
/// @brief @copybrief components::State

#include <string_view>
#include <unordered_set>

USERVER_NAMESPACE_BEGIN

namespace components {

class ComponentContext;

namespace impl {
class ComponentContextImpl;
}

// clang-format off
/// @brief All components pass through these stages during the service lifetime.
/// @see @ref scripts/docs/en/userver/component_system.md
///
/// @dot
/// digraph ServiceLifetimeStages {
/// node [shape=record];
///
/// kLoading [label="{kLoading | <f0> * Components are constructed }"];
/// kOnAllComponentsLoadedIsRunning [label="{kOnAllComponentsLoadedIsRunning | <f1> * OnAllComponentsLoaded is called }"];
/// kRunning [label="{kRunning | <f2> * All components loaded successfully \n * Service is fully operational }"];
/// kGracefulShutdown [label="{kGracefulShutdown | <f3> * Waits graceful_shutdown_interval }"];
/// kOnAllComponentsAreStoppingIsRunning [label="{kOnAllComponentsAreStoppingIsRunning | <f4> * OnAllComponentsAreStopping is called \n * Reverse-dependency order }"];
/// kStopping [label="{kStopping | <f5> * Components are destroyed \n * Reverse-dependency order }"];
///
/// kLoading -> kOnAllComponentsLoadedIsRunning;
/// kLoading -> kOnAllComponentsAreStoppingIsRunning [label=" Exception during construction "];
/// kOnAllComponentsLoadedIsRunning -> kRunning;
/// kOnAllComponentsLoadedIsRunning -> kOnAllComponentsAreStoppingIsRunning [label=" OnAllComponentsLoaded throws "];
/// kRunning -> kGracefulShutdown [label=" Received SIGINT or SIGTERM "];
/// kGracefulShutdown -> kOnAllComponentsAreStoppingIsRunning;
/// kOnAllComponentsAreStoppingIsRunning -> kStopping;
/// }
/// @enddot
// clang-format on
enum class ServiceLifetimeStage {
    /// Constructors are running for all registered components. Components can depend on each other at this stage
    /// by calling @ref components::ComponentContext::FindComponent and friends.
    ///
    /// If any component throws an exception, then the service transitions
    /// into @ref ServiceLifetimeStage::kOnAllComponentsAreStoppingIsRunning stage.
    kLoading,

    /// @ref components::ComponentBase::OnAllComponentsLoaded (noop by default) is running for all components.
    /// This stage starts after constructors for all components have completed without an exception.
    ///
    /// The order of `OnAllComponentsLoaded` hooks invocations respects the order of components defined
    /// at @ref ServiceLifetimeStage::kLoading stage.
    kOnAllComponentsLoadedIsRunning,

    /// This stage marks that all `OnAllComponentsLoaded` hooks (as described
    /// in @ref ServiceLifetimeStage::kOnAllComponentsLoadedIsRunning) have completed
    /// successfully (without an exception). At this point the service is fully running.
    ///
    /// This stage ends once the service receives a shutdown signal (`SIGINT` or `SIGTERM`).
    kRunning,

    /// The service waits for `graceful_shutdown_interval` (0 by default) before continuing with the actual service
    /// shutdown in @ref ServiceLifetimeStage::kOnAllComponentsAreStoppingIsRunning.
    ///
    /// @see @ref components::ManagerControllerComponent
    ///
    /// Example:
    /// @snippet core/functional_tests/graceful_shutdown/static_config.yaml graceful_shutdown_interval
    kGracefulShutdown,

    /// @ref components::ComponentBase::OnAllComponentsAreStopping (noop by default) is running for all components.
    /// This stage starts once the service has received a shutdown signal (see @ref ServiceLifetimeStage::kRunning) and
    /// @ref ServiceLifetimeStage::kGracefulShutdown stage (if any) has completed.
    ///
    /// If an error occurs during service startup, then `OnAllComponentsAreStopping` runs after
    /// @ref components::ComponentBase::OnLoadingCancelled for all constructed components.
    ///
    /// The order of `OnAllComponentsAreStopping` hooks invocations respects the order of components defined
    /// at @ref ServiceLifetimeStage::kLoading stage (they run in the reverse-dependency order).
    kOnAllComponentsAreStoppingIsRunning,

    /// Destructors are running for all components. This stage starts once
    /// @ref ServiceLifetimeStage::kOnAllComponentsAreStoppingIsRunning stage.
    ///
    /// If an error occurs during service startup, then destructors run
    /// after @ref ServiceLifetimeStage::kOnAllComponentsAreStoppingIsRunning for all constructed components.
    ///
    /// The order of destructor invocations respects the order of components defined
    /// at @ref ServiceLifetimeStage::kLoading stage (they run in the reverse-dependency order).
    kStopping,
};

/// Converts a @ref components::ServiceLifetimeStage to debug string for logging.
std::string_view ToString(ServiceLifetimeStage);

/// A view of the components' state that is usable after the components are
/// constructed and until all the components are destroyed.
///
/// @see components::ComponentContext
class State final {
public:
    explicit State(const ComponentContext& cc) noexcept;

    /// @returns true if one of the components is in fatal state and can not
    /// work. A component is in fatal state if the
    /// components::ComponentHealth::kFatal value is returned from the overridden
    /// components::ComponentBase::GetComponentHealth().
    bool IsAnyComponentInFatalState() const;

    /// @returns the current service lifetime stage.
    /// @see @ref components::ServiceLifetimeStage
    ServiceLifetimeStage GetServiceLifetimeStage() const;

    /// @returns true if component with name `component_name` depends
    /// (directly or transitively) on a component with name `dependency`.
    ///
    /// Component with name `component_name` should be loaded.
    /// Components construction should finish before any call to this function
    /// is made.
    ///
    /// Note that GetAllDependencies usually is more effective, if you are
    /// planning multiple calls for the same component name.
    bool HasDependencyOn(std::string_view component_name, std::string_view dependency) const;

    /// @returns all the components that `component_name` depends on directly or
    /// transitively.
    ///
    /// Component with name `component_name` should be loaded.
    /// Components construction should finish before any call to this function
    /// is made. The result should now outlive the all the components
    /// destruction.
    std::unordered_set<std::string_view> GetAllDependencies(std::string_view component_name) const;

private:
    const impl::ComponentContextImpl& impl_;
};

}  // namespace components

USERVER_NAMESPACE_END
