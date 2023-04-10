#pragma once

/// @file userver/storages/postgres/component.hpp
/// @brief @copybrief components::Postgres

#include <chrono>
#include <vector>

#include <userver/components/loggable_component_base.hpp>
#include <userver/concurrent/async_event_source.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/utils/statistics/entry.hpp>

#include <userver/storages/postgres/database.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief PosgreSQL client component
///
/// Provides access to a PostgreSQL cluster.
///
/// ## Dynamic options:
/// * @ref POSTGRES_DEFAULT_COMMAND_CONTROL
/// * @ref POSTGRES_HANDLERS_COMMAND_CONTROL
/// * @ref POSTGRES_QUERIES_COMMAND_CONTROL
/// * @ref POSTGRES_CONNECTION_POOL_SETTINGS
/// * @ref POSTGRES_CONNECTION_SETTINGS
/// * @ref POSTGRES_CONNECTION_PIPELINE_ENABLED
/// * @ref POSTGRES_STATEMENT_METRICS_SETTINGS
/// * @ref POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED
///
/// ## Static configuration example:
///
/// ```
///  # yaml
///  postgres-taxi:
///    dbalias: taxi
///    blocking_task_processor: task-processor-name
///    max_replication_lag: 60s
///    min_pool_size: 4
///    max_pool_size: 15
///    max_queue_size: 200
///    max_statement_metrics: 50
/// ```
/// You must specify either `dbalias` or `conn_info`.
/// If the component is configured with an alias, it will lookup connection data
/// in secdist.json
///
/// You must specify `blocking_task_processor` as well.
///
/// `max_replication_lag` can be used to tune replication lag limit for replicas.
/// Once the replica lag exceeds this value it will be automatically disabled.
/// Note, however, that client-size lag detection is not precise in nature
/// and can only provide the precision of couple seconds.
///
/// ## Secdist format
///
/// A PosgreSQL alias in secdist is described as a JSON array of objects
/// containing a single cluster description. There are two formats of describing
/// a cluster, the first one assigns predefined roles to DSNs, the second one
/// is just a list of DSNs and the Postgres component takes care of discovering
/// the cluster's topology itself.
///
/// ### Predefined roles
///
/// In predefined roles format the component requires single-host connection
/// strings.
///
/// ```json
/// {
///   "shard_number" : 0,
///   "master": "host=localhost dbname=example",
///   "sync_slave": "host=localhost dbname=example",
///   "slaves": [
///     "host=localhost dbname=example"
///   ]
/// }
/// ```
///
/// The predefined roles format is deprecated and the support will be removed
/// soon.
///
/// ### Automatic discovery
///
/// In automatic discovery format the connection strings are any valid
/// PostgreSQL connection strings including multi-host ones with the exception
/// of `target_session_attrs` which will be ignored.
///
/// ```json
/// {
///   "shard_number" : 0,
///   "hosts": [
///     "host=host1,host2,host3 dbname=example",
///     "postgresql://host1:5432,host2:6432,host3:12000/example"
///   ]
/// }
/// ```
///
/// The `shard_number` parameter is required in both formats and must match the
/// index of cluster description object in the alias array.
///
/// Please see [PostgreSQL documentation](https://www.postgresql.org/docs/12/libpq-connect.html#LIBPQ-CONNSTRING)
/// on connection strings.
///
/// ## Static options:
/// Name                    | Description                                               | Default value
/// ----------------------- | --------------------------------------------------------- | -------------
/// dbalias                 | name of the database in secdist config (if available)     | --
/// dbconnection            | connection DSN string (used if no dbalias specified)      | --
/// blocking_task_processor | name of task processor for background blocking operations | --
/// max_replication_lag     | replication lag limit for usable slaves                   | 60s
/// min_pool_size           | number of connections created initially                   | 4
/// max_pool_size           | limit of connections count                                | 15
/// sync-start              | perform initial connections synchronously                 | false
/// dns_resolver            | server hostname resolver type (getaddrinfo or async)      | 'async'
/// persistent-prepared-statements | cache prepared statements or not                   | true
/// user-types-enabled      | disabling will disallow use of user-defined types         | true
/// ignore_unused_query_params| disable check for not-NULL query params that are not used in query| false
/// monitoring-dbalias      | name of the database for monitorings                      | calculated from dbalias or dbconnection options
/// max_prepared_cache_size | prepared statements cache size limit                      | 5000
/// max_statement_metrics   | limit of exported metrics for named statements            | 0
/// min_pool_size           | number of connections created initially                   | 4
/// max_pool_size           | maximum number of created connections                     | 15
/// max_queue_size          | maximum number of clients waiting for a connection        | 200
/// connecting_limit        | limit for concurrent establishing connections number per pool (0 - unlimited) | 0
/// connlimit_mode          | max_connections setup mode (manual or auto)               | auto
/// error-injection         | artificial error injection settings, error_injection::Settings | --

// clang-format on

class Postgres : public LoggableComponentBase {
 public:
  static constexpr auto kDefaultMaxReplicationLag = std::chrono::seconds{60};
  /// Default shard number
  static constexpr size_t kDefaultShardNumber = 0;
  /// Default command control
  static constexpr storages::postgres::CommandControl kDefaultCommandControl{
      std::chrono::milliseconds{500},  // network timeout
      std::chrono::milliseconds{250}   // statement timeout
  };

  /// Component constructor
  Postgres(const ComponentConfig&, const ComponentContext&);
  /// Component destructor
  ~Postgres() override;

  /// Cluster accessor for default shard number
  storages::postgres::ClusterPtr GetCluster() const;

  /// Cluster accessor for specific shard number
  storages::postgres::ClusterPtr GetClusterForShard(size_t shard) const;

  /// Get total shard count
  size_t GetShardCount() const;

  /// Get database object
  storages::postgres::DatabasePtr GetDatabase() const { return database_; }

  /// Reports statistics for PostgreSQL driver
  void ExtendStatistics(utils::statistics::Writer& writer);

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  void OnConfigUpdate(const dynamic_config::Snapshot& cfg);

  concurrent::AsyncEventSubscriberScope config_subscription_;

  utils::statistics::Entry statistics_holder_;

  std::string name_;
  std::string db_name_;
  storages::postgres::ClusterSettings initial_settings_;
  storages::postgres::DatabasePtr database_;
};

template <>
inline constexpr bool kHasValidate<Postgres> = true;

}  // namespace components

USERVER_NAMESPACE_END
