#pragma once

/// @file userver/storages/postgres/cluster.hpp
/// @brief @copybrief storages::postgres::Cluster

#include <memory>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/error_injection/settings_fwd.hpp>
#include <userver/testsuite/postgres_control.hpp>
#include <userver/testsuite/tasks.hpp>

#include <userver/storages/postgres/cluster_types.hpp>
#include <userver/storages/postgres/database.hpp>
#include <userver/storages/postgres/detail/non_transaction.hpp>
#include <userver/storages/postgres/options.hpp>
#include <userver/storages/postgres/query.hpp>
#include <userver/storages/postgres/statistics.hpp>
#include <userver/storages/postgres/transaction.hpp>

/// @page pg_topology uPg: Cluster topology discovery
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
///
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref pg_errors | @ref pg_user_types ⇨
/// @htmlonly </div> @endhtmlonly

USERVER_NAMESPACE_BEGIN

namespace components {
class Postgres;
}  // namespace components

namespace storages::postgres {

namespace detail {

class ClusterImpl;
using ClusterImplPtr = std::unique_ptr<ClusterImpl>;

}  // namespace detail

/// @ingroup userver_clients
///
/// @brief Interface for executing queries on a cluster of PostgreSQL servers
///
/// See @ref pg_user_row_types "Typed PostgreSQL results" for usage examples of
/// the storages::postgres::ResultSet.
///
/// Usually retrieved from components::Postgres component.
///
/// @todo Add information about topology discovery
class Cluster {
 public:
  /// Cluster constructor
  /// @param dsns List of DSNs to connect to
  /// @param resolver asynchronous DNS resolver
  /// @param bg_task_processor task processor for blocking connection operations
  /// @param cluster_settings struct with settings fields:
  /// task_data_keys_settings - settings for per-handler command controls
  /// topology_settings - settings for host discovery
  /// pool_settings - settings for connection pools
  /// conn_settings - settings for individual connections
  /// @param default_cmd_ctls default command execution options
  /// @param testsuite_pg_ctl command execution options customizer for testsuite
  /// @param ei_settings error injection settings
  /// @note When `max_connection_pool_size` is reached, and no idle connections
  /// available, `PoolError` is thrown for every new connection
  /// request
  Cluster(DsnList dsns, clients::dns::Resolver* resolver,
          engine::TaskProcessor& bg_task_processor,
          const ClusterSettings& cluster_settings,
          DefaultCommandControls&& default_cmd_ctls,
          const testsuite::PostgresControl& testsuite_pg_ctl,
          const error_injection::Settings& ei_settings,
          testsuite::TestsuiteTasks& testsuite_tasks,
          dynamic_config::Source config_source);
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

  /// Start a named transaction in any available connection depending on
  /// transaction options.
  ///
  /// If the transaction is RW, will start transaction in a connection
  /// to master. If the transaction is RO, will start trying connections
  /// starting with slaves.
  /// `name` is used to set command control in config at runtime.
  /// @throws ClusterUnavailable if no hosts are available
  Transaction Begin(const std::string& name, const TransactionOptions&);

  /// Start a named transaction in a connection with specified host selection
  /// rules.
  ///
  /// If the requested host role is not available, may fall back to another
  /// host role, see ClusterHostType.
  /// If the transaction is RW, only master connection can be used.
  /// `name` is used to set command control in config at runtime.
  /// @throws ClusterUnavailable if no hosts are available
  Transaction Begin(const std::string& name, ClusterHostTypeFlags,
                    const TransactionOptions&);
  /// @}

  /// @name Single-statement query in an auto-commit transaction
  /// @{

  /// @brief Execute a statement at host of specified type.
  /// @note You must specify at least one role from ClusterHostType here
  ///
  /// @snippet storages/postgres/tests/landing_test.cpp Exec sample
  ///
  /// @warning Do NOT create a query string manually by embedding arguments!
  /// It leads to vulnerabilities and bad performance. Either pass arguments
  /// separately, or use storages::postgres::ParameterScope.
  template <typename... Args>
  ResultSet Execute(ClusterHostTypeFlags, const Query& query,
                    const Args&... args);

  /// @brief Execute a statement with specified host selection rules and command
  /// control settings.
  /// @note You must specify at least one role from ClusterHostType here
  ///
  /// @warning Do NOT create a query string manually by embedding arguments!
  /// It leads to vulnerabilities and bad performance. Either pass arguments
  /// separately, or use storages::postgres::ParameterScope.
  template <typename... Args>
  ResultSet Execute(ClusterHostTypeFlags, OptionalCommandControl,
                    const Query& query, const Args&... args);

  /// @brief Execute a statement with stored arguments and specified host
  /// selection rules.
  ResultSet Execute(ClusterHostTypeFlags flags, const Query& query,
                    const ParameterStore& store);

  /// @brief Execute a statement with stored arguments, specified host selection
  /// rules and command control settings.
  ResultSet Execute(ClusterHostTypeFlags flags,
                    OptionalCommandControl statement_cmd_ctl,
                    const Query& query, const ParameterStore& store);
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

  /// Replaces cluster connection settings.
  ///
  /// Connections with an old settings will be dropped and reestablished.
  void SetConnectionSettings(const ConnectionSettings& settings);

  void SetPoolSettings(const PoolSettings& settings);

  void SetStatementMetricsSettings(const StatementMetricsSettings& settings);

 private:
  detail::NonTransaction Start(ClusterHostTypeFlags, OptionalCommandControl);

  OptionalCommandControl GetQueryCmdCtl(const std::string& query_name) const;
  OptionalCommandControl GetHandlersCmdCtl(
      OptionalCommandControl cmd_ctl) const;

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
  if (!statement_cmd_ctl && query.GetName()) {
    statement_cmd_ctl = GetQueryCmdCtl(query.GetName()->GetUnderlying());
  }
  statement_cmd_ctl = GetHandlersCmdCtl(statement_cmd_ctl);
  auto ntrx = Start(flags, statement_cmd_ctl);
  return ntrx.Execute(statement_cmd_ctl, query, args...);
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
