#pragma once

/// @file userver/storages/postgres/dist_lock_component_base.hpp
/// @brief @copybrief storages::postgres::DistLockComponentBase

#include <userver/components/loggable_component_base.hpp>
#include <userver/dist_lock/dist_locked_worker.hpp>
#include <userver/storages/postgres/dist_lock_strategy.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

// clang-format off

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
///
/// ## Static options:
/// name           | Description  | Default value
/// -------------- | ------------ | -------------
/// cluster        | postgres cluster name | --
/// table          | table name to store distlocks | --
/// lockname       | name of the lock | --
/// lock-ttl       | TTL of the lock; must be at least as long as the duration between subsequent cancellation checks, otherwise brain split is possible | --
/// pg-timeout     | timeout, must be less than lock-ttl/2 | --
/// restart-delay  | how much time to wait after failed task restart | 100ms
/// autostart      | if true, start automatically after component load | true
/// task-processor | the name of the TaskProcessor for running DoWork | main-task-processor
/// testsuite-support | Enable testsuite support | false
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

// clang-format on

class DistLockComponentBase : public components::LoggableComponentBase {
 public:
  DistLockComponentBase(const components::ComponentConfig&,
                        const components::ComponentContext&);

  ~DistLockComponentBase() override;

  dist_lock::DistLockedWorker& GetWorker();

  static yaml_config::Schema GetStaticConfigSchema();

 protected:
  /// Override this function with anything that must be done under the pg lock.
  ///
  /// ## Example implementation
  ///
  /// ```cpp
  /// void MyDistLockComponent::DoWork()
  /// {
  ///     while (!engine::ShouldCancel())
  ///     {
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

 private:
  std::unique_ptr<dist_lock::DistLockedWorker> worker_;
  USERVER_NAMESPACE::utils::statistics::Entry statistics_holder_;
  bool autostart_;
  bool testsuite_enabled_{false};
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
