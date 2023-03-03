#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <userver/dynamic_config/source.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/storages/secdist/fwd.hpp>
#include <userver/utils/statistics/fwd.hpp>

#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

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

  /// @cond
  MultiMongo(std::string name, const storages::secdist::Secdist& secdist,
             storages::mongo::PoolConfig pool_config,
             clients::dns::Resolver* dns_resolver,
             dynamic_config::Source config_source);
  /// @endcond

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

  /// Writes statistics
  friend void DumpMetric(utils::statistics::Writer& writer,
                         const MultiMongo& multi_mongo);

  const std::string& GetName() const { return name_; }

 private:
  storages::mongo::PoolPtr FindPool(const std::string& dbalias) const;

  const std::string name_;
  const storages::secdist::Secdist& secdist_;
  dynamic_config::Source config_source_;
  const storages::mongo::PoolConfig pool_config_;
  clients::dns::Resolver* dns_resolver_;
  rcu::Variable<PoolMap> pool_map_;
};

}  // namespace storages::mongo

USERVER_NAMESPACE_END
