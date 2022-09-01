#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <boost/lockfree/queue.hpp>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/semaphore.hpp>
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
#include <storages/postgres/detail/statement_timings_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

class ConnectionPool : public std::enable_shared_from_this<ConnectionPool> {
  class EmplaceEnabler;

 public:
  ConnectionPool(EmplaceEnabler, Dsn dsn, clients::dns::Resolver* resolver,
                 engine::TaskProcessor& bg_task_processor,
                 const std::string& db_name, const PoolSettings& settings,
                 const ConnectionSettings& conn_settings,
                 const StatementMetricsSettings& statement_metrics_settings,
                 const DefaultCommandControls& default_cmd_ctls,
                 const testsuite::PostgresControl& testsuite_pg_ctl,
                 error_injection::Settings ei_settings);

  ~ConnectionPool();

  static std::shared_ptr<ConnectionPool> Create(
      Dsn dsn, clients::dns::Resolver* resolver,
      engine::TaskProcessor& bg_task_processor, const std::string& db_name,
      const InitMode& init_mode, const PoolSettings& pool_settings,
      const ConnectionSettings& conn_settings,
      const StatementMetricsSettings& statement_metrics_settings,
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

  void SetSettings(const PoolSettings& settings);

  void SetConnectionSettings(const ConnectionSettings& settings);

  void SetStatementMetricsSettings(const StatementMetricsSettings& settings);

  const detail::StatementTimingsStorage& GetStatementTimingsStorage() const {
    return sts_;
  }

 private:
  using SizeGuard = USERVER_NAMESPACE::utils::SizeGuard<std::atomic<size_t>>;
  using SharedCounter = std::shared_ptr<std::atomic<size_t>>;
  using SharedSizeGuard = USERVER_NAMESPACE::utils::SizeGuard<SharedCounter>;

  void Init(InitMode mode);

  TimeoutDuration GetExecuteTimeout(OptionalCommandControl) const;

  [[nodiscard]] engine::TaskWithResult<bool> Connect(SharedSizeGuard&&);

  void TryCreateConnectionAsync();
  void CheckMinPoolSizeUnderflow();

  void Push(Connection* connection);
  Connection* Pop(engine::Deadline);

  void Clear();

  void DeleteConnection(Connection* connection);
  void DeleteBrokenConnection(Connection* connection);
  void DropOutdatedConnection(Connection* connection);

  void AccountConnectionStats(Connection::Statistics stats);

  Connection* AcquireImmediate();
  void MaintainConnections();
  void StartMaintainTask();
  void StopMaintainTask();

  using RecentCounter = USERVER_NAMESPACE::utils::statistics::RecentPeriod<
      USERVER_NAMESPACE::utils::statistics::RelaxedCounter<size_t>, size_t>;

  mutable InstanceStatistics stats_;
  Dsn dsn_;
  clients::dns::Resolver* resolver_;
  std::string db_name_;
  rcu::Variable<PoolSettings> settings_;
  rcu::Variable<ConnectionSettings> conn_settings_;
  engine::TaskProcessor& bg_task_processor_;
  USERVER_NAMESPACE::utils::PeriodicTask ping_task_;
  engine::Mutex wait_mutex_;
  engine::ConditionVariable conn_available_;
  boost::lockfree::queue<Connection*> queue_;
  SharedCounter size_;
  engine::Semaphore connecting_semaphore_;
  std::atomic<size_t> wait_count_;
  DefaultCommandControls default_cmd_ctls_;
  testsuite::PostgresControl testsuite_pg_ctl_;
  const error_injection::Settings ei_settings_;
  RecentCounter recent_conn_errors_;
  USERVER_NAMESPACE::utils::TokenBucket cancel_limit_;
  detail::StatementTimingsStorage sts_;
};

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
