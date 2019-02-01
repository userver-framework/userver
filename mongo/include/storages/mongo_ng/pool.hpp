#pragma once

/// @file storages/mongo/pool.hpp
/// @brief @copybrief storages::mongo_ng::Pool

#include <memory>
#include <string>

#include <storages/mongo_ng/pool_config.hpp>

namespace storages::mongo_ng {

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

  // Returns a handle for the specified collection from the default database
  // Collection GetCollection(std::string name);

  // Returns a handle for the specified database
  // Database GetDatabase(std::string database);
  // XXX: list databases?

 private:
  std::shared_ptr<impl::PoolImpl> impl_;
};

using PoolPtr = std::shared_ptr<Pool>;

}  // namespace storages::mongo_ng
