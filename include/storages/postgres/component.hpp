#pragma once

/// @file storages/postgres/component.hpp
/// @brief @copybrief components::Postgres

#include <memory>

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>

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

  /// Component constructor
  Postgres(const ComponentConfig&, const ComponentContext&);
  /// Component destructor
  ~Postgres();

  /// Client cluster accessor
  storages::postgres::ClusterPtr GetCluster() const;

 private:
  std::unique_ptr<engine::TaskProcessor> bg_task_processor_;
  storages::postgres::ClusterPtr cluster_;
};

}  // namespace components
