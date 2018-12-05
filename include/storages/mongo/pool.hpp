#pragma once

/// @file storages/mongo/pool.hpp
/// @brief @copybrief storages::mongo::Pool

#include <memory>
#include <stdexcept>
#include <string>

#include <storages/mongo/collection.hpp>
#include <storages/mongo/pool_config.hpp>

namespace engine {
class TaskProcessor;
}  // namespace engine

namespace storages {
namespace mongo {

namespace impl {
class PoolImpl;
}  // namespace impl

/// MongoDB client pool
class Pool {
 public:
  /// Client pool constructor
  /// @param uri database connection string
  /// @param task_processor TaskProcessor to use for synchronous operations
  /// @param config pool configuration
  Pool(const std::string& uri, engine::TaskProcessor& task_processor,
       const PoolConfig& config);
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
