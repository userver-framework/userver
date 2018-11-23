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
/// ```
/// {
///   postgres-taxi:
///     dbalias: taxi
///     blocking_task_processor: task-processor-name
///     min_pool_size: 16
///     max_pool_size: 100
/// }
/// ```
/// You must specify either `dbalias` or `conn_info`.
/// You must specify `blocking_task_processor` as well.
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

  size_t min_pool_size_ = 0;
  size_t max_pool_size_ = 0;
  engine::TaskProcessor* bg_task_processor_ = nullptr;
  storages::postgres::ShardedClusterDescription shard_to_desc_;
  mutable engine::Mutex shards_mutex_;
  mutable std::vector<storages::postgres::ClusterPtr> shards_;
  mutable engine::Mutex shards_ready_mutex_;
  mutable std::vector<storages::postgres::Cluster*> shards_ready_;
};

}  // namespace components
