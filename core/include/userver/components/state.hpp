#pragma once

/// @file userver/components/state.hpp
/// @brief @copybrief components::State

#include <string_view>
#include <unordered_set>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace components {

class ComponentContext;

enum class ComponentHealth;

namespace impl {
class ComponentContextImpl;
}

// clang-format off
/// @brief All components pass through these stages during the service lifetime.
/// @see @ref scripts/docs/en/userver/component_system.md
///
/// @dot
/// digraph ServiceLifetimeStages {
/// rankdir=TB;
/// nodesep=0.4;
/// ranksep=0.5;
/// node [shape=box, fontsize=11];
/// edge [fontsize=10];
///
/// kLoading [group=states, width=4.75, label=< <B>kLoading</B><BR/><FONT POINT-SIZE="5">&nbsp;</FONT><BR/>Components are constructed. >];
/// kOnAllComponentsLoadedIsRunning [group=states, width=4.75, label=< <B>kOnAllComponentsLoadedIsRunning</B><BR/><FONT POINT-SIZE="5">&nbsp;</FONT><BR/>OnAllComponentsLoaded is called. >];
/// kRunning [group=states, width=4.75, label=< <B>kRunning</B><BR/><FONT POINT-SIZE="5">&nbsp;</FONT><BR/>All components loaded successfully.<BR/>Service is fully operational. >];
/// GracefulShutdownDecision [shape=diamond, label="Graceful\nshutdown?"];
/// kGracefulShutdown [group=states, width=4.75, label=< <B>kGracefulShutdown</B><BR/><FONT POINT-SIZE="5">&nbsp;</FONT><BR/>Waits for<BR/>graceful_shutdown_continue_accepting_requests_interval.<BR/>Then OnGracefulShutdown is called. >];
/// kOnAllComponentsAreStoppingIsRunning [group=states, width=4.75, label=< <B>kOnAllComponentsAreStoppingIsRunning</B><BR/><FONT POINT-SIZE="5">&nbsp;</FONT><BR/>OnAllComponentsAreStopping is called.<BR/>Hooks run in reverse-dependency order. >];
/// kStopping [group=states, width=4.75, label=< <B>kStopping</B><BR/><FONT POINT-SIZE="5">&nbsp;</FONT><BR/>Components are destroyed.<BR/>Destructors run in reverse-dependency order. >];
///
/// exc1Top [shape=none, width=0.01, height=0.01, group=exc1, label=""];
/// exc1Run [shape=none, width=0.01, height=0.01, group=exc1, label=""];
/// exc1Bot [shape=none, width=0.01, height=0.01, group=exc1, label=""];
/// exc2Top [shape=none, width=0.01, height=0.01, group=exc2, label=""];
/// exc2Bot [shape=none, width=0.01, height=0.01, group=exc2, label=""];
///
/// kLoading -> kOnAllComponentsLoadedIsRunning [weight=100];
/// kOnAllComponentsLoadedIsRunning -> kRunning [weight=100];
/// kRunning -> kGracefulShutdown [penwidth=0, arrowhead=none, weight=100];
/// kGracefulShutdown -> kOnAllComponentsAreStoppingIsRunning [weight=100];
/// kOnAllComponentsAreStoppingIsRunning -> kStopping [weight=100];
/// kRunning:e -> GracefulShutdownDecision:n [label="Received SIGINT\nor SIGTERM"];
/// {rank=same; kGracefulShutdown; GracefulShutdownDecision}
/// kGracefulShutdown -> GracefulShutdownDecision [penwidth=0, arrowhead=none, weight=10];
/// GracefulShutdownDecision -> kGracefulShutdown [label="yes", constraint=false];
/// GracefulShutdownDecision -> kOnAllComponentsAreStoppingIsRunning [label="no", constraint=false];
///
/// {rank=same; exc1Top; kOnAllComponentsLoadedIsRunning}
/// {rank=same; exc1Run; exc2Top; kRunning}
/// {rank=same; exc1Bot; exc2Bot; kGracefulShutdown}
/// exc1Top -> kOnAllComponentsLoadedIsRunning [penwidth=0, arrowhead=none, weight=50];
/// exc1Run -> exc2Top [penwidth=0, arrowhead=none, weight=40];
/// exc2Top -> kRunning [penwidth=0, arrowhead=none, weight=50];
/// exc1Bot -> exc2Bot [penwidth=0, arrowhead=none, weight=40];
/// exc2Bot -> kGracefulShutdown [penwidth=0, arrowhead=none, weight=50];
///
/// kLoading -> exc1Top [arrowhead=none];
/// exc1Top -> exc1Run [arrowhead=none, label="Exception during\nconstruction"];
/// exc1Run -> exc1Bot [arrowhead=none];
/// exc1Bot -> kOnAllComponentsAreStoppingIsRunning;
/// kOnAllComponentsLoadedIsRunning -> exc2Top [arrowhead=none];
/// exc2Top -> exc2Bot [arrowhead=none, label="OnAllComponentsLoaded\nthrows"];
/// exc2Bot -> kOnAllComponentsAreStoppingIsRunning;
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

    /// The service performs a graceful shutdown if either `graceful_shutdown_continue_accepting_requests_interval`
    /// or `graceful_shutdown_pending_requests_completion_interval` is non-zero.
    /// First, it waits for `graceful_shutdown_pending_requests_completion_interval` unless the interval is zero.
    /// Second, it stops accepting new requests and continues processing of already running requests for
    /// `graceful_shutdown_pending_requests_completion_interval` unless the interval is zero.
    /// Then the normal shutdown procedure continues in @ref ServiceLifetimeStage::kOnAllComponentsAreStoppingIsRunning.
    ///
    /// @see @ref scripts/docs/en/userver/graceful_shutdown.md
    /// @see @ref components::ManagerControllerComponent
    ///
    /// Example:
    /// @snippet core/functional_tests/graceful_shutdown/static_config.yaml graceful_shutdown_settings
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
    /// Component name together with its current health.
    struct ComponentWithHealth {
        std::string_view name;
        ComponentHealth health;
    };

    explicit State(const ComponentContext& cc) noexcept;

    /// @returns true if one of the components is in fatal state and can not
    /// work. A component is in fatal state if the
    /// components::ComponentHealth::kFatal value is returned from the overridden
    /// components::ComponentBase::GetComponentHealth().
    bool IsAnyComponentInFatalState() const;

    /// @returns all components that are not in
    /// components::ComponentHealth::kOk state.
    ///
    /// Components construction should finish before any call to this function
    /// is made. The result should not outlive the components destruction.
    std::vector<ComponentWithHealth> GetUnhealthyComponents() const;

    /// @returns the current service lifetime stage.
    /// @see @ref components::ServiceLifetimeStage
    ServiceLifetimeStage GetServiceLifetimeStage() const;

    /// @returns true if the service is being shut down gracefully.
    bool IsInGracefulShutdown() const;

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
