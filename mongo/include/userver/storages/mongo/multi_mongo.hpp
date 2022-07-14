#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <userver/rcu/rcu.hpp>
#include <userver/utils/swappingsmart.hpp>

#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages {

namespace secdist {
class Secdist;
}  // namespace secdist

namespace mongo {

class MultiMongo {
  using PoolMap = std::unordered_map<std::string, storages::mongo::PoolPtr>;

 public:
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

    /// @brief Replaces the working database set
    void Activate();

   private:
    MultiMongo* target_{nullptr};
    std::shared_ptr<PoolMap> pool_map_ptr_;
  };

  MultiMongo(std::string name, const storages::secdist::Secdist& secdist,
             storages::mongo::PoolConfig pool_config,
             clients::dns::Resolver* dns_resolver, Config mongo_config);

  /// @brief Client pool accessor
  /// @param dbalias name previously passed to `AddPool`
  /// @throws PoolNotFoundException if no such database is enabled
  storages::mongo::PoolPtr GetPool(const std::string& dbalias) const;

  /// @brief Adds a database to the working set by its name.
  /// Equivalent to
  /// `NewPoolSet()`-`AddExistingPools()`-`AddPool(dbalias)`-`Activate()`
  /// @param dbalias name of the database in secdist config
  void AddPool(std::string dbalias);

  /// @brief Removes the database with the specified name from the working set.
  /// Equivalent to
  /// `NewPoolSet()`-`AddExistingPools()`-`RemovePool(dbalias)`-`Activate()`
  /// @param dbalias name of the database passed to AddPool
  /// @returns whether the database was in the working set
  bool RemovePool(const std::string& dbalias);

  /// Creates an empty database set bound to the current MultiMongo instance
  PoolSet NewPoolSet();

  /// Returns JSON with statistics
  formats::json::Value GetStatistics(bool verbose) const;

  const std::string& GetName() const { return name_; }

  void SetConfig(Config config);

 private:
  storages::mongo::PoolPtr FindPool(const std::string& dbalias) const;

  Config GetConfigCopy() const;

  const std::string name_;
  const storages::secdist::Secdist& secdist_;
  std::unique_ptr<rcu::Variable<Config>> config_storage_;
  const storages::mongo::PoolConfig pool_config_;
  clients::dns::Resolver* dns_resolver_;
  utils::SwappingSmart<PoolMap> pool_map_ptr_;
};

}  // namespace mongo
}  // namespace storages

USERVER_NAMESPACE_END
