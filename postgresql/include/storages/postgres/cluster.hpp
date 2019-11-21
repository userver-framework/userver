#pragma once

#include <memory>

#include <engine/task/task_with_result.hpp>

#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/detail/non_transaction.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/statistics.hpp>
#include <storages/postgres/transaction.hpp>

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

namespace storages {
namespace postgres {

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
  /// @param cluster_desc Cluster configuration description
  /// @param bg_task_processor task processor for blocking connection operations
  /// @param pool_settings settings for connection pools
  /// @param conn_settings settings for individual connections
  /// @param cmd_ctl command execution options
  /// @note When `max_connection_pool_size` is reached, and no idle connections
  /// available, `PoolError` is thrown for every new connection
  /// request
  Cluster(const ClusterDescription& cluster_desc,
          engine::TaskProcessor& bg_task_processor, PoolSettings pool_settings,
          ConnectionSettings conn_settings, CommandControl cmd_ctl);
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
                    Args&&... args);
  /// Execute a statement at host of specified type with specific command
  /// control settings.
  /// @note ClusterHostType::kAny cannot be used here
  template <typename... Args>
  ResultSet Execute(ClusterHostType, CommandControl,
                    const std::string& statement, Args&&... args);
  //@}

  void SetDefaultCommandControl(CommandControl);
  SharedCommandControl GetDefaultCommandControl() const;

 private:
  detail::NonTransaction Start(ClusterHostType ht, engine::Deadline deadline);

 private:
  friend class components::Postgres;
  engine::TaskWithResult<void> DiscoverTopology();

  detail::ClusterImplPtr pimpl_;
};

template <typename... Args>
ResultSet Cluster::Execute(ClusterHostType ht, const std::string& statement,
                           Args&&... args) {
  auto deadline =
      engine::Deadline::FromDuration(GetDefaultCommandControl()->execute);
  auto ntrx = Start(ht, deadline);
  return ntrx.Execute(statement, std::forward<Args>(args)...);
}

template <typename... Args>
ResultSet Cluster::Execute(ClusterHostType ht, CommandControl statement_cmd_ctl,
                           const std::string& statement, Args&&... args) {
  auto deadline = engine::Deadline::FromDuration(statement_cmd_ctl.execute);
  auto ntrx = Start(ht, deadline);
  return ntrx.Execute(std::move(statement_cmd_ctl), statement,
                      std::forward<Args>(args)...);
}

}  // namespace postgres
}  // namespace storages
