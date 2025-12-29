#pragma once

/// @file userver/storages/postgres/dist_lock_component_base.hpp
/// @brief @copybrief storages::postgres::DistLockComponentBase

#include <userver/components/component_base.hpp>
#include <userver/dist_lock/dist_locked_worker.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/storages/postgres/dist_lock_strategy.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// @ingroup userver_components userver_base_classes
///
/// @brief Base class for postgres-based distlock worker components
///
/// A component that implements a distlock with lock in Postgres. Inherit from
/// DistLockComponentBase and implement DoWork(). Lock options are configured
/// in static config.
///
/// The class must be used for infinite loop jobs. If you want a distributed
/// periodic, you should look at locked_periodiccomponents::PgLockedPeriodic.
///
/// @see dist_lock::DistLockedTask
/// @see locked_periodiccomponents::PgLockedPeriodic
///
/// ## Static configuration example:
///
/// ```yaml
///        example-distlock:
///            cluster: postgresql-service
///            table: service.distlocks
///            lockname: master
///            pg-timeout: 1s
///            lock-ttl: 10s
///            autostart: true
/// ```
/// See config `POSTGRES_DISTLOCK_SETTINGS`, some of parameters can be dynamically overridden.
///
/// ## Static options of storages::postgres::DistLockComponentBase :
/// @include{doc} scripts/docs/en/components_schema/postgresql/src/storages/postgres/dist_lock_component_base.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// ## Migration example
///
/// You have to create a SQL table for distlocks. An example of the migration
/// script is as following:
///
/// ```SQL
/// CREATE TABLE service.distlocks
/// (
///     key             TEXT PRIMARY KEY,
///     owner           TEXT,
///     expiration_time TIMESTAMPTZ
/// );
/// ```
///
/// @see @ref scripts/docs/en/userver/periodics.md
class DistLockComponentBase : public components::ComponentBase {
public:
    DistLockComponentBase(const components::ComponentConfig&, const components::ComponentContext&);

    ~DistLockComponentBase() override;

    dist_lock::DistLockedWorker& GetWorker();

    /// @note In testsuite always returns `true`, because there is only one host.
    bool OwnsLock() const noexcept;

    static yaml_config::Schema GetStaticConfigSchema();

protected:
    /// Override this function with anything that must be done under the pg lock.
    ///
    /// ## Example implementation
    ///
    /// ```cpp
    /// void MyDistLockComponent::DoWork()
    /// {
    ///     // `IsCancelAdvised` is advisory/soft signal to stop the task.
    ///     // Check it in every independent processing iteration.
    ///     // Whereas @ref engine::current_task::ShouldCancel() checks as frequently,
    ///     // as you can to honor low-level task cancellation.
    ///     while (!IsCancelAdvised())
    ///     {
    ///         // Start a new trace_id
    ///         auto span = tracing::Span::MakeRootSpan("my-dist-lock");
    ///
    ///         // If Foo() or other function in DoWork() throws an exception,
    ///         // DoWork() will be restarted in `restart-delay` seconds.
    ///         Foo();
    ///
    ///         // Check for cancellation after cpu-intensive Foo().
    ///         // You must check for cancellation at least every `lock-ttl`
    ///         // seconds to have time to notice lock prolongation failure.
    ///         if (engine::ShouldCancel()) break;
    ///
    ///         Bar();
    ///     }
    /// }
    /// ```
    ///
    /// @note `DoWork` must honour task cancellation and stop ASAP when
    /// it is cancelled, otherwise brain split is possible (IOW, two different
    /// users do work assuming both of them hold the lock, which is not true).
    virtual void DoWork() = 0;

    /// Override this function to provide custom testsuite handler.
    virtual void DoWorkTestsuite() { DoWork(); }

    /// Must be called in ctr
    void AutostartDistLock();

    /// Must be called in dtr
    void StopDistLock();

    /// Check this method when going for the next independent processing
    /// iteration. Whereas @ref engine::current_task::ShouldCancel()
    /// checks as frequently, as you can to honor low-level task cancellation.
    bool IsCancelAdvised() const;

private:
    bool ShouldRunOnHost(const dynamic_config::Snapshot& config) const;
    void OnConfigUpdate(const dynamic_config::Diff& diff);

    dynamic_config::Source config_;
    const std::string name_;
    const std::string real_host_name_;
    std::unique_ptr<dist_lock::DistLockedWorker> worker_;
    bool autostart_;
    bool testsuite_enabled_{false};
    dist_lock::DistLockSettings default_settings_;

    concurrent::AsyncEventSubscriberScope subscription_token_;
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
