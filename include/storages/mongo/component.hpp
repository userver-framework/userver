#pragma once

/// @file storages/mongo/component.hpp
/// @brief @copybrief components::Mongo

#include <memory>

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>

#include "pool.hpp"

namespace engine {
class TaskProcessor;
}  // namespace engine

namespace components {

// clang-format off

/// @brief MongoDB client component
///
/// Provides access to a MongoDB database.
///
/// ## Configuration example:
///
/// ```
/// {
///   "name": "mongo-taxi",
///   "dbalias": "taxi",
///   "conn_timeout_ms": 5000,
///   "so_timeout_ms": 30000,
///   "min_pool_size": 32,
///   "max_pool_size": 256,
///   "threads": "2"
/// }
/// ```
/// You must specify one of `dbalias` or `dbconnection`.
///
/// ## Available options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// dbalias | name of the database in secdist config (if available) | --
/// dbconnection | connection string (used if no dbalias specified) | --
/// conn_timeout_ms | connection timeout (ms) | 5s
/// so_timeout_ms | socket timeout (ms) | 30s
/// min_pool_size | number of connections created initially | 32
/// max_pool_size | limit of idle connections count | 256
/// threads | size of backing thread pool | 2
///
/// The limit for the total connections count is storages::mongo::kCriticalPoolSize.

// clang-format on

class Mongo : public LoggableComponentBase {
 public:
  /// Default connection timeout
  static constexpr int kDefaultConnTimeoutMs = 5 * 1000;
  /// Default socket timeout
  static constexpr int kDefaultSoTimeoutMs = 30 * 1000;
  /// Default initial connection count
  static constexpr size_t kDefaultMinPoolSize = 32;
  /// Default idle connections limit
  static constexpr size_t kDefaultMaxPoolSize = 256;
  /// Default thread pool size
  static constexpr size_t kDefaultThreadsNum = 2;

  /// Component constructor
  Mongo(const ComponentConfig&, const ComponentContext&);
  /// Component destructor
  ~Mongo();

  /// Client pool accessor
  storages::mongo::PoolPtr GetPool() const;

 private:
  std::unique_ptr<engine::TaskProcessor> task_processor_;
  storages::mongo::PoolPtr pool_;
};

}  // namespace components
