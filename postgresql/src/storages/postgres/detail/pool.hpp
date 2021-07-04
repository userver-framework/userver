#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <boost/lockfree/queue.hpp>

#include <userver/engine/condition_variable.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/error_injection/settings.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/testsuite/postgres_control.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/utils/token_bucket.hpp>
#include <utils/size_guard.hpp>

#include <storages/postgres/default_command_controls.hpp>
#include <userver/storages/postgres/detail/connection_ptr.hpp>
#include <userver/storages/postgres/detail/non_transaction.hpp>
#include <userver/storages/postgres/options.hpp>
#include <userver/storages/postgres/statistics.hpp>
#include <userver/storages/postgres/transaction.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/pg_impl_types.hpp>

namespace storages::postgres::detail {

class ConnectionPool : public std::enable_shared_from_this<ConnectionPool> {
  class EmplaceEnabler;

 public:
  ConnectionPool(EmplaceEnabler, Dsn dsn,
                 engine::TaskProcessor& bg_task_processor,
                 const PoolSettings& settings,
                 const ConnectionSettings& conn_settings,
                 const DefaultCommandControls& default_cmd_ctls,
                 const testsuite::PostgresControl& testsuite_pg_ctl,
                 error_injection::Settings ei_settings);

  ~ConnectionPool();

  static std::shared_ptr<ConnectionPool> Create(
      Dsn dsn, engine::TaskProcessor& bg_task_processor,
      const PoolSettings& pool_settings,
      const ConnectionSettings& conn_settings,
      const DefaultCommandControls& default_cmd_ctls,
      const testsuite::PostgresControl& testsuite_pg_ctl,
      error_injection::Settings ei_settings);

  [[nodiscard]] ConnectionPtr Acquire(engine::Deadline);
  void Release(Connection* connection);

  const InstanceStatistics& GetStatistics() const;
  [[nodiscard]] Transaction Begin(const TransactionOptions& options,
                                  OptionalCommandControl trx_cmd_ctl = {});

  [[nodiscard]] NonTransaction Start(OptionalCommandControl cmd_ctl = {});

  CommandControl GetDefaultCommandControl() const;

 private:
  using SizeGuard = ::utils::SizeGuard<std::atomic<size_t>>;
  using SharedCounter = std::shared_ptr<std::atomic<size_t>>;
  using SharedSizeGuard = ::utils::SizeGuard<SharedCounter>;

  void Init();

  TimeoutDuration GetExecuteTimeout(OptionalCommandControl);

  [[nodiscard]] engine::TaskWithResult<bool> Connect(SharedSizeGuard&&);

  void TryCreateConnectionAsync();
  void CheckMinPoolSizeUnderflow();

  void Push(Connection* connection);
  Connection* Pop(engine::Deadline);

  void Clear();

  void DeleteConnection(Connection* connection);
  void DeleteBrokenConnection(Connection* connection);

  void AccountConnectionStats(Connection::Statistics stats);

  Connection* AcquireImmediate();
  void MaintainConnections();
  void StartMaintainTask();
  void StopMaintainTask();

 private:
  using RecentCounter = ::utils::statistics::RecentPeriod<
      ::utils::statistics::RelaxedCounter<size_t>, size_t>;

  mutable InstanceStatistics stats_;
  Dsn dsn_;
  PoolSettings settings_;
  ConnectionSettings conn_settings_;
  engine::TaskProcessor& bg_task_processor_;
  ::utils::PeriodicTask ping_task_;
  engine::Mutex wait_mutex_;
  engine::ConditionVariable conn_available_;
  boost::lockfree::queue<Connection*> queue_;
  SharedCounter size_;
  std::atomic<size_t> wait_count_;
  DefaultCommandControls default_cmd_ctls_;
  testsuite::PostgresControl testsuite_pg_ctl_;
  const error_injection::Settings ei_settings_;
  RecentCounter recent_conn_errors_;
  ::utils::TokenBucket cancel_limit_;
};

}  // namespace storages::postgres::detail
