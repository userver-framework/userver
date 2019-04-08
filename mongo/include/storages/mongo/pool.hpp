#pragma once

/// @file storages/mongo/pool.hpp
/// @brief @copybrief storages::mongo::Pool

#include <memory>
#include <string>

#include <storages/mongo/collection.hpp>
#include <storages/mongo/pool_config.hpp>

namespace storages::mongo {

namespace impl {
class PoolImpl;
}  // namespace impl

/// MongoDB client pool
class Pool {
 public:
  /// Client pool constructor
  /// @param id pool identificaton string
  /// @param uri database connection string
  /// @param config pool configuration
  Pool(std::string id, const std::string& uri, const PoolConfig& config);
  ~Pool();

  /// Checks whether a collection exists
  bool HasCollection(const std::string& name) const;

  /// Returns a handle for the specified collection
  Collection GetCollection(std::string name) const;

 private:
  std::shared_ptr<impl::PoolImpl> impl_;
};

using PoolPtr = std::shared_ptr<Pool>;

}  // namespace storages::mongo
