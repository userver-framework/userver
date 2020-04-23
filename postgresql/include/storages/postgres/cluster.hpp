#pragma once

/// @file storages/postgres/cluster.hpp
/// @brief @copybrief storages::postgres::Cluster

#include <memory>

#include <engine/task/task_with_result.hpp>
#include <error_injection/settings_fwd.hpp>

#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/detail/non_transaction.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/statistics.hpp>
#include <storages/postgres/transaction.hpp>
#include <testsuite/postgres_control.hpp>

/// @page pg_topology µPg: Cluster topology discovery
///
/// @par Principles of PgaaS role determination
/// - Every host except master is in recovery state from PostgreSQL's POV.
/// This means the check 'select pg_is_in_recovery()' returns `false` for the
/// master and `true` for every other host type.
/// - Some hosts are in sync slave mode. This may be determined by executing
/// 'show synchronous_standby_names' on the master.
/// Slave names returned by this statement are escaped host names where every
/// non-letter and non-digit are replaced with '_'.
///
/// @par PgaaS sync slaves lag
/// By default, PgaaS synchronous slaves are working with 'synchronous_commit'
/// set to 'remote_apply'. Therefore, sync slave may be lagging behind the
/// master and thus is not truly 'synchronous' from the reader's POV,
/// but things may change with time.
///
/// @par Algorithm
/// Every topology update runs for at most 80% of timer update interval, but no
/// less than kMinCheckDuration.
/// Topology check is done in CheckTopology method in two phases:
/// - host check routine which collects relevant info about host states and
/// roles is executed
/// - update routine which actualizes host states is called
/// All the processing is done single-thread, and CheckTopology method is not
/// designed to support multi-threaded calls, so synchronization must be
/// guaranteed outside.
/// Check routine consists of the following stages:
/// - reconnect/wait for connection to become available
/// - availability check (joint with master detection)
/// - sync slaves detection (only for master host)
/// Every stage, including connection to the host, may fail and thus cause
/// reconnect again and again.
/// The core of check process is task multiplexing that allows to simultaneously
/// track different tasks representing different stages.
/// During checks the info about hosts is stored into changes.last_check_type
/// variable.
/// The info is then actualized later during second phase of topology check.

namespace engine {
class TaskProcessor;
}  // namespace engine

namespace components {
class Postgres;
}  // namespace components

namespace storages::postgres {

namespace detail {

class ClusterImpl;
using ClusterImplPtr = std::unique_ptr<ClusterImpl>;

}  // namespace detail

/// @brief Interface for executing queries on a cluster of PostgreSQL servers
///
/// @todo Add information about topology discovery
class Cluster {
 public:
  /// Cluster constructor
  /// @param dsns List of DSNs to connect to
  /// @param bg_task_processor task processor for blocking connection operations
  /// @param pool_settings settings for connection pools
  /// @param conn_settings settings for individual connections
  /// @param default_cmd_ctl default command execution options
  /// @param testsuite_pg_ctl command execution options customizer for testsuite
  /// @param ei_settings error injection settings
  /// @note When `max_connection_pool_size` is reached, and no idle connections
  /// available, `PoolError` is thrown for every new connection
  /// request
  Cluster(DsnList dsns, engine::TaskProcessor& bg_task_processor,
          const PoolSettings& pool_settings,
          const ConnectionSettings& conn_settings,
          const CommandControl& default_cmd_ctl,
          const testsuite::PostgresControl& testsuite_pg_ctl,
          const error_injection::Settings& ei_settings);
  ~Cluster();

  /// Get cluster statistics
  ///
  /// The statistics object is too big to fit on stack
  ClusterStatisticsPtr GetStatistics() const;

  //@{
  /** @name Transaction start */
  /// Start a transaction in any available connection depending on transaction
  /// options. If the transaction is RW, will start transaction in a connection
  /// to master. If the transaction is RO, will start trying connections
  /// starting with slaves.
  /// @throws ClusterUnavailable if no hosts are available
  Transaction Begin(const TransactionOptions&, OptionalCommandControl = {});
  /// Start a transaction in a connection to a specific cluster part
  /// If specified host type is not available, may fall back to other host type,
  /// @see ClusterHostType.
  /// If the transaction is RW, only master connection will be used.
  /// @throws ClusterUnavailable if no hosts are available
  Transaction Begin(ClusterHostType, const TransactionOptions&,
                    OptionalCommandControl = {});
  //@}

  //@{
  /** @name Single-statement query in an auto-commit transaction */
  /// Execute a statement at host of specified type.
  /// @note ClusterHostType::kAny cannot be used here
  template <typename... Args>
  ResultSet Execute(ClusterHostType, const std::string& statement,
                    const Args&... args);
  /// Execute a statement at host of specified type with specific command
  /// control settings.
  /// @note ClusterHostType::kAny cannot be used here
  template <typename... Args>
  ResultSet Execute(ClusterHostType, OptionalCommandControl,
                    const std::string& statement, const Args&... args);
  //@}

  /// Replaces globally updated command control with a static user-provided one
  void SetDefaultCommandControl(CommandControl);

  /// Returns current default command control
  CommandControl GetDefaultCommandControl() const;

  /// @cond
  /// Updates default command control from global config (if not set by user)
  void ApplyGlobalCommandControlUpdate(CommandControl);
  /// @endcond

 private:
  detail::NonTransaction Start(ClusterHostType, OptionalCommandControl);

 private:
  detail::ClusterImplPtr pimpl_;
};

template <typename... Args>
ResultSet Cluster::Execute(ClusterHostType ht, const std::string& statement,
                           const Args&... args) {
  return Execute(ht, OptionalCommandControl{}, statement, args...);
}

template <typename... Args>
ResultSet Cluster::Execute(ClusterHostType ht,
                           OptionalCommandControl statement_cmd_ctl,
                           const std::string& statement, const Args&... args) {
  auto ntrx = Start(ht, statement_cmd_ctl);
  return ntrx.Execute(statement_cmd_ctl, statement, args...);
}

}  // namespace storages::postgres
