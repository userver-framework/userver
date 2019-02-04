#pragma once

/// @file storages/postgres/component.hpp
/// @brief @copybrief components::Postgres

#include <vector>

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/statistics_storage.hpp>
#include <engine/mutex.hpp>
#include <utils/statistics/storage.hpp>

#include <storages/postgres/cluster_types.hpp>
#include <storages/postgres/postgres_fwd.hpp>

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
///    min_pool_size: 16
///    max_pool_size: 100
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
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// dbalias | name of the database in secdist config (if available) | --
/// dbconnection | connection DSN string (used if no dbalias specified) | --
/// blocking_task_processor | name of task processor for background blocking operations | --
/// min_pool_size | number of connections created initially | 16
/// max_pool_size | limit of connections count | 100

// clang-format on

class Postgres : public LoggableComponentBase {
 public:
  /// Default initial connection count
  static constexpr size_t kDefaultMinPoolSize = 16;
  /// Default connections limit
  static constexpr size_t kDefaultMaxPoolSize = 100;
  /// Default shard number
  static constexpr size_t kDefaultShardNumber = 0;

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

  /// Reports statistics for PostgreSQL driver
  formats::json::Value ExtendStatistics(
      const utils::statistics::StatisticsRequest& /*request*/);

 private:
  components::StatisticsStorage& statistics_storage_;
  utils::statistics::Entry statistics_holder_;

  std::string db_name_;
  size_t min_pool_size_ = 0;
  size_t max_pool_size_ = 0;
  engine::TaskProcessor* bg_task_processor_ = nullptr;
  storages::postgres::ShardedClusterDescription cluster_desc_;
  std::vector<storages::postgres::ClusterPtr> clusters_;
};

}  // namespace components
