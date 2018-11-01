#pragma once

/// @file storages/postgres/component.hpp
/// @brief @copybrief components::Postgres

#include <memory>

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <engine/mutex.hpp>

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
/// ```
/// {
///   postgres-taxi:
///     dbalias: taxi
///     blocking_task_processor_threads: 2
///     min_pool_size: 16
///     max_pool_size: 100
/// }
/// ```
/// You must specify either `dbalias` or `conn_info`.
/// You must specify `blocking_task_processor_threads` as well.
///
/// ## Available options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// dbalias | name of the database in secdist config (if available) | --
/// dbconnection | connection DSN string (used if no dbalias specified) | --
/// blocking_task_processor_threads | size of thread pool for background blocking operations | --
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

 private:
  size_t min_pool_size_ = 0;
  size_t max_pool_size_ = 0;
  std::unique_ptr<engine::TaskProcessor> bg_task_processor_;
  storages::postgres::ShardedClusterDescription shard_to_desc_;
  mutable engine::Mutex shard_cluster_mutex_;
  mutable std::unordered_map<size_t, storages::postgres::ClusterPtr>
      shard_to_cluster_;
};

}  // namespace components
