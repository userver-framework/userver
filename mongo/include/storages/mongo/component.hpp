#pragma once

/// @file storages/mongo/component.hpp
/// @brief @copybrief components::Mongo

#include <memory>
#include <unordered_map>

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <storages/mongo/pool.hpp>
#include <storages/mongo/pool_config.hpp>
#include <storages/secdist/component.hpp>
#include <utils/swappingsmart.hpp>

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
/// The limit for the total connections count is storages::mongo::PoolConfig::kCriticalSize.

// clang-format on

class Mongo : public LoggableComponentBase {
 public:
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

// clang-format off

/// @brief Dynamically configurable MongoDB client component
///
/// Provides acces to a dynamically reconfigurable set of MongoDB databases.
///
/// ## Configuration example:
///
/// ```
/// {
///   "name": "mongo-dynamic",
///   "conn_timeout_ms": 5000,
///   "so_timeout_ms": 30000,
///   "min_pool_size": 32,
///   "max_pool_size": 256,
///   "threads": "2"
/// }
/// ```
/// ## Available options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// conn_timeout_ms | connection timeout (ms) | 5s
/// so_timeout_ms | socket timeout (ms) | 30s
/// min_pool_size | number of connections created initially (per database) | 32
/// max_pool_size | limit of idle connections count (per database) | 256
/// threads | size of backing thread pool (shared) | 4
///
/// The limit for the connections count per database is storages::mongo::PoolConfig::kCriticalSize.

// clang-format on

class MultiMongo : public LoggableComponentBase {
 private:
  using PoolMap = std::unordered_map<std::string, storages::mongo::PoolPtr>;

 public:
  static constexpr const char* kName = "multi-mongo";

  /// Default shared thread pool size
  static constexpr size_t kDefaultThreadsNum = 4;

  /// Component constructor
  MultiMongo(const ComponentConfig&, const ComponentContext&);
  /// Component destructor
  ~MultiMongo();

  /// Client pool accessor
  /// @param dbalias name previously passed to `AddPool`
  /// @throws PoolNotFound if no such database is enabled
  storages::mongo::PoolPtr GetPool(const std::string& dbalias) const;

  /// @brief Adds a database to the working set by its name
  /// Equivalent to
  /// `NewPoolSet()`-`AddExistingPools()`-`AddPool(dbalias)`-`Activate()`
  /// @param dbalias name of the database in secdist config
  void AddPool(std::string dbalias);

  /// @brief Removes the database with the specified name from the working set
  /// Equivalent to
  /// `NewPoolSet()`-`AddExistingPools()`-`RemovePool(dbalias)`-`Activate()`
  /// @param dbalias name of the database passed to AddPool
  /// @returns whether the database was in the working set
  bool RemovePool(const std::string& dbalias);

  /// Database set builder
  class PoolSet {
   public:
    explicit PoolSet(MultiMongo&);

    PoolSet(const PoolSet&);
    PoolSet(PoolSet&&) noexcept;
    PoolSet& operator=(const PoolSet&);
    PoolSet& operator=(PoolSet&&) noexcept;

    /// Adds all currently enabled databases to the set
    void AddExistingPools();

    /// @brief Adds a database to the set by its name
    /// @param dbalias name of the database in secdist config
    void AddPool(std::string dbalias);

    /// @brief Removes the database with the specified name from the set
    /// @param dbalias name of the database passed to AddPool
    /// @returns whether the database was in the set
    bool RemovePool(const std::string& dbalias);

    /// @brief Replaces the working database set of the component
    void Activate();

   private:
    MultiMongo* target_;
    std::shared_ptr<PoolMap> pool_map_ptr_;
  };

  /// Creates an empty database set bound to the component
  PoolSet NewPoolSet();

 private:
  storages::mongo::PoolPtr FindPool(const std::string& dbalias) const;

  const Secdist& secdist_;
  const storages::mongo::PoolConfig pool_config_;
  std::unique_ptr<engine::TaskProcessor> task_processor_;
  utils::SwappingSmart<PoolMap> pool_map_ptr_;
};

}  // namespace components
