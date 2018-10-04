#pragma once

/// @file storages/mongo/pool.hpp
/// @brief @copybrief storages::mongo::Pool

#include <memory>
#include <stdexcept>
#include <string>

#include <storages/mongo/collection.hpp>
#include <storages/mongo/mongo.hpp>

namespace engine {
class TaskProcessor;
}  // namespace engine

namespace storages {
namespace mongo {

namespace impl {
class PoolImpl;
}  // namespace impl

/// Per-pool connection limit
constexpr size_t kCriticalPoolSize = 1000;

/// Represents client pool errors
class PoolError : public MongoError {
  using MongoError::MongoError;
};

/// Represents config validation errors
class InvalidConfig : public PoolError {
  using PoolError::PoolError;
};

/// MongoDB client pool
class Pool {
 public:
  /// Client pool constructor
  /// @param uri database connection string
  /// @param task_processor TaskProcessor to use for synchronous operations
  /// @param conn_timeout_ms connection timeout (ms)
  /// @param so_timeout_ms socket timeout (ms)
  /// @param min_size minimum (initial) idle connections count
  /// @param max_size maximum idle connections count
  /// @note Absolute maximum number of connections for a single pool is fixed
  /// and equals storages::mongo::kCriticalPoolSize.
  Pool(const std::string& uri, engine::TaskProcessor& task_processor,
       int conn_timeout_ms, int so_timeout_ms, size_t min_size,
       size_t max_size);
  ~Pool();

  /// Returns an interface for the specified collection from default database
  Collection GetCollection(std::string collection_name);

  /// Return an interface for the specified collection/database pair
  Collection GetCollection(std::string database_name,
                           std::string collection_name);

 private:
  std::shared_ptr<impl::PoolImpl> impl_;
};

using PoolPtr = std::shared_ptr<Pool>;

}  // namespace mongo
}  // namespace storages
