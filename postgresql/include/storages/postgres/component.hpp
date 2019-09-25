#pragma once

/// @file storages/postgres/component.hpp
/// @brief @copybrief components::Postgres

#include <chrono>
#include <vector>

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/loggable_component_base.hpp>
#include <components/statistics_storage.hpp>
#include <engine/mutex.hpp>
#include <taxi_config/storage/component.hpp>
#include <utils/statistics/storage.hpp>

#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/database.hpp>

namespace engine {
class TaskProcessor;
}  // namespace engine

namespace components {

// clang-format off

/// @brief PosgreSQL client component
///
/// Provides access to a PostgreSQL cluster.
///
/// ## Configuration example:
///
/// ```yaml
///  postgres-taxi:
///    dbalias: taxi
///    blocking_task_processor: task-processor-name
///    min_pool_size: 4
///    max_pool_size: 15
///    max_queue_size: 200
/// ```
/// You must specify either `dbalias` or `conn_info`.
/// If the component is configured with an alias, it will lookup connection data
/// in secdist.json
///
/// You must specify `blocking_task_processor` as well.
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
/// The predefined roles format is deprecated and the support will be removed as
/// soon as all µservices migrate to the automatical discovery format.
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
/// Please see [PostgreSQL documentation](https://www.postgresql.org/docs/10/libpq-connect.html#LIBPQ-CONNSTRING)
/// on connection strings.
///
/// ## Available options:
/// Name                    | Description                                               | Default value
/// ----------------------- | --------------------------------------------------------- | -------------
/// dbalias                 | name of the database in secdist config (if available)     | --
/// dbconnection            | connection DSN string (used if no dbalias specified)      | --
/// blocking_task_processor | name of task processor for background blocking operations | --
/// min_pool_size           | number of connections created initially                   | 4
/// max_pool_size           | limit of connections count                                | 15
/// persistent-prepared-statements | cache prepared statements or not                   | true

// clang-format on

class Postgres : public LoggableComponentBase {
 public:
  /// Default initial connection count
  static constexpr size_t kDefaultMinPoolSize = 4;
  /// Default connections limit
  static constexpr size_t kDefaultMaxPoolSize = 15;
  /// Default size of queue for clients waiting for connections
  static constexpr size_t kDefaultMaxQueueSize = 200;
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
  ~Postgres();

  /// Cluster accessor for default shard number
  storages::postgres::ClusterPtr GetCluster() const;

  /// Cluster accessor for specific shard number
  storages::postgres::ClusterPtr GetClusterForShard(size_t shard) const;

  /// Get total shard count
  size_t GetShardCount() const;

  /// Get database object
  storages::postgres::DatabasePtr GetDatabase() const { return database_; }

  /// Reports statistics for PostgreSQL driver
  formats::json::Value ExtendStatistics(
      const utils::statistics::StatisticsRequest& /*request*/);

 private:
  using TaxiConfigPtr = std::shared_ptr<const taxi_config::Config>;
  storages::postgres::CommandControl GetCommandControlConfig(
      const TaxiConfigPtr& cfg) const;
  void OnConfigUpdate(const TaxiConfigPtr& cfg);
  /// Set default command control for all clusters managed by the component
  void SetDefaultCommandControl(storages::postgres::CommandControl);

 private:
  utils::AsyncEventSubscriberScope config_subscription_;

  components::StatisticsStorage& statistics_storage_;
  utils::statistics::Entry statistics_holder_;

  std::string db_name_;
  storages::postgres::PoolSettings pool_settings_;
  storages::postgres::ConnectionSettings conn_settings_;
  engine::TaskProcessor* bg_task_processor_ = nullptr;
  storages::postgres::ShardedClusterDescription cluster_desc_;
  storages::postgres::DatabasePtr database_;
};

}  // namespace components
