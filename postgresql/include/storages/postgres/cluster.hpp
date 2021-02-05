#pragma once

/// @file storages/postgres/cluster.hpp
/// @brief @copybrief storages::postgres::Cluster

#include <memory>

#include <engine/task/task_processor_fwd.hpp>
#include <engine/task/task_with_result.hpp>
#include <error_injection/settings_fwd.hpp>
#include <testsuite/postgres_control.hpp>

#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/detail/non_transaction.hpp>
#include <storages/postgres/options.hpp>
#include <storages/postgres/query.hpp>
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
/// See
/// https://www.postgresql.org/docs/current/runtime-config-replication.html#GUC-SYNCHRONOUS-STANDBY-NAMES
/// for more information.
///
/// @par PgaaS sync slaves lag
/// By default, PgaaS synchronous slaves are working with 'synchronous_commit'
/// set to 'remote_apply'. Therefore, sync slave may be lagging behind the
/// master and thus is not truly 'synchronous' from the reader's POV,
/// but things may change with time.
///
/// @par Implementation
/// Topology update runs every second.
///
/// Every host is assighed a connection with special ID (4100200300).
/// Using this connection we check for host availability, writability
/// (master detection) and perform RTT measurements.
///
/// After the initial check we know about master presence and RTT for each host.
/// Master host is queried about synchronous replication status. We use this
/// info to identify synchronous slaves and to detect "quorum commit" presence.

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
  /// @param topology_settings settings for host discovery
  /// @param pool_settings settings for connection pools
  /// @param conn_settings settings for individual connections
  /// @param default_cmd_ctls default command execution options
  /// @param testsuite_pg_ctl command execution options customizer for testsuite
  /// @param ei_settings error injection settings
  /// @note When `max_connection_pool_size` is reached, and no idle connections
  /// available, `PoolError` is thrown for every new connection
  /// request
  Cluster(DsnList dsns, engine::TaskProcessor& bg_task_processor,
          const TopologySettings& topology_settings,
          const PoolSettings& pool_settings,
          const ConnectionSettings& conn_settings,
          DefaultCommandControls&& default_cmd_ctls,
          const testsuite::PostgresControl& testsuite_pg_ctl,
          const error_injection::Settings& ei_settings);
  ~Cluster();

  /// Get cluster statistics
  ///
  /// The statistics object is too big to fit on stack
  ClusterStatisticsPtr GetStatistics() const;

  /// @name Transaction start
  /// @{

  /// Start a transaction in any available connection depending on transaction
  /// options.
  ///
  /// If the transaction is RW, will start transaction in a connection
  /// to master. If the transaction is RO, will start trying connections
  /// starting with slaves.
  /// @throws ClusterUnavailable if no hosts are available
  Transaction Begin(const TransactionOptions&, OptionalCommandControl = {});

  /// Start a transaction in a connection with specified host selection rules.
  ///
  /// If the requested host role is not available, may fall back to another
  /// host role, see ClusterHostType.
  /// If the transaction is RW, only master connection can be used.
  /// @throws ClusterUnavailable if no hosts are available
  Transaction Begin(ClusterHostTypeFlags, const TransactionOptions&,
                    OptionalCommandControl = {});
  /// @}

  /// @name Single-statement query in an auto-commit transaction
  /// @{

  /// @brief Execute a statement at host of specified type.
  /// @note You must specify at least one role from ClusterHostType here
  template <typename... Args>
  ResultSet Execute(ClusterHostTypeFlags, const Query& query,
                    const Args&... args);

  /// @brief Execute a statement with specified host selection rules and command
  /// control settings.
  /// @note You must specify at least one role from ClusterHostType here
  template <typename... Args>
  ResultSet Execute(ClusterHostTypeFlags, OptionalCommandControl,
                    const Query& query, const Args&... args);
  /// @}

  /// Replaces globally updated command control with a static user-provided one
  void SetDefaultCommandControl(CommandControl);

  /// Returns current default command control
  CommandControl GetDefaultCommandControl() const;

  void SetHandlersCommandControl(
      const CommandControlByHandlerMap& handlers_command_control);

  void SetQueriesCommandControl(
      const CommandControlByQueryMap& queries_command_control);

  /// @cond
  /// Updates default command control from global config (if not set by user)
  void ApplyGlobalCommandControlUpdate(CommandControl);
  /// @endcond

 private:
  detail::NonTransaction Start(ClusterHostTypeFlags, OptionalCommandControl);

  OptionalCommandControl GetQueryCmdCtl(
      const std::optional<Query::Name>& query_name) const;

 private:
  detail::ClusterImplPtr pimpl_;
};

template <typename... Args>
ResultSet Cluster::Execute(ClusterHostTypeFlags flags, const Query& query,
                           const Args&... args) {
  return Execute(flags, OptionalCommandControl{}, query, args...);
}

template <typename... Args>
ResultSet Cluster::Execute(ClusterHostTypeFlags flags,
                           OptionalCommandControl statement_cmd_ctl,
                           const Query& query, const Args&... args) {
  if (!statement_cmd_ctl) {
    statement_cmd_ctl = GetQueryCmdCtl(query.GetName());
  }
  auto ntrx = Start(flags, statement_cmd_ctl);
  return ntrx.Execute(statement_cmd_ctl, query.Statement(), args...);
}

}  // namespace storages::postgres
