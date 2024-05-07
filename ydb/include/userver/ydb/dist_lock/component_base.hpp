#pragma once

/// @file userver/ydb/dist_lock/component_base.hpp
/// @brief @copybrief ydb::DistLockComponentBase

#include <optional>
#include <string>

#include <userver/components/loggable_component_base.hpp>
#include <userver/utils/statistics/entry.hpp>
#include <userver/ydb/dist_lock/worker.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {
class TestsuiteTasks;
}  // namespace testsuite

namespace ydb {

// clang-format off

/// @ingroup userver_components userver_base_classes
///
/// @brief Base class for YDB-based distlock worker components
///
/// A component that implements a distlock with lock in YDB. Inherit from
/// DistLockComponentBase and implement DoWork(). Lock options are configured
/// in static config.
///
/// The class must be used for infinite loop jobs.
///
/// ## Static configuration example:
///
/// @snippet ydb/functional_tests/basic/static_config.yaml  sample-dist-lock
///
/// ## Static options:
/// name           | Description  | Default value
/// -------------- | ------------ | -------------
/// semaphore-name | name of the semaphore within the coordination node | --
/// database-settings.dbname | the key of the database within ydb component (NOT the actual database path) | --
/// database-settings.coordination-node | name of the coordination node within the database | --
/// initial-setup | if true, then create the coordination node and the semaphore unless they already exist | true
/// task-processor | the name of the TaskProcessor for running DoWork | main-task-processor
/// node-settings.session-grace-period | the time after which the lock will be given to another host after a network failure | 10s
/// session-timeout | for how long we will try to restore session after a network failure before dropping it | 5s
/// restart-session-delay | backoff before attempting to reconnect session after it returns "permanent failure" | 1s
/// acquire-interval | backoff before repeating a failed Acquire call | 100ms
/// restart-delay | backoff before calling DoWork again after it returns or throws | 100ms
/// cancel-task-time-limit | time, within which a cancelled DoWork is expected to finish | 5s
///
/// @see @ref scripts/docs/en/userver/periodics.md

// clang-format on
class DistLockComponentBase : public components::LoggableComponentBase {
 public:
  DistLockComponentBase(const components::ComponentConfig&,
                        const components::ComponentContext&);
  ~DistLockComponentBase() override;

  /// @brief Checks if the the current service instance owns the lock.
  ///
  /// Useful for:
  ///
  /// 1. Writing metrics only on the primary (for the given distlock) host.
  ///
  /// 2. Running on-demand work, e.g. in handlers, only on the primary host.
  ///    The work must be short-lived (single requests with small timeout),
  ///    otherwise brain split
  ///    (where multiple hosts consider themselves primary) is possible.
  bool OwnsLock() const noexcept;

  static yaml_config::Schema GetStaticConfigSchema();

 protected:
  /// Override this function with anything that must be done under the lock.
  ///
  /// @note `DoWork` must honour task cancellation and stop ASAP when
  /// it is cancelled, otherwise brain split is possible (IOW, two different
  /// users do work assuming both of them hold the lock, which is not true).
  ///
  /// @note When DoWork exits for any reason, the lock is dropped, then after
  /// `restart-delay` the lock is attempted to be re-acquired (but by that time
  /// another host likely acquires the lock).
  ///
  /// ## Example implementation
  ///
  /// @snippet ydb/functional_tests/basic/ydb_service.cpp  DoWork
  virtual void DoWork() = 0;

  /// Override this function to provide a custom testsuite handler, e.g. one
  /// that does not contain a "while not cancelled" loop.
  /// Calls `DoWork` by default.
  ///
  /// In testsuite runs, the normal DoWork execution disabled by default.
  /// There is an API to call DoWorkTestsuite from testsuite, imitating waiting
  /// until DoWork runs:
  ///
  /// @snippet ydb/functional_tests/basic/tests/test_distlock.py  run
  virtual void DoWorkTestsuite() { DoWork(); }

  /// Must be called at the end of the constructor of the derived component.
  void Start();

  /// Must be called in the destructor of the derived component.
  void Stop() noexcept;

 private:
  testsuite::TestsuiteTasks& testsuite_tasks_;
  const std::string testsuite_task_name_;

  // Worker contains a task that may refer to other fields, so it must be right
  // before subscriptions. Add new fields above this comment.
  std::optional<DistLockedWorker> worker_;

  // Subscriptions must be the last fields.
  utils::statistics::Entry statistics_holder_;
};

}  // namespace ydb

USERVER_NAMESPACE_END
