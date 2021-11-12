#pragma once

/// @file userver/storages/mongo/pool.hpp
/// @brief @copybrief storages::mongo::Pool

#include <memory>
#include <string>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/pool_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

namespace impl {
class PoolImpl;
}  // namespace impl

class Config;

/// @ingroup userver_clients
///
/// @brief MongoDB client pool.
///
/// Use constucor only for tests, in production the pool should be retrieved
/// from @ref userver_components "the components" via
/// components::Mongo::GetPool() or components::MultiMongo::GetPool().
///
/// ## Example usage:
///
/// @snippet storages/mongo/collection_mongotest.cpp  Sample Mongo usage
class Pool {
 public:
  /// Client pool constructor
  /// @param id pool identificaton string
  /// @param uri database connection string
  /// @param config pool configuration
  Pool(std::string id, const std::string& uri, const PoolConfig& pool_config,
       clients::dns::Resolver* dns_resolver, const Config& mongo_config);
  ~Pool();

  /// Checks whether a collection exists
  bool HasCollection(const std::string& name) const;

  /// Returns a handle for the specified collection
  Collection GetCollection(std::string name) const;

  /// Returns pool statistics JSON
  formats::json::Value GetStatistics() const;

  /// Returns verbose pool statistics JSON (with separate metrics for ops/RP/WC)
  formats::json::Value GetVerboseStatistics() const;

  void SetConfig(const Config& config);

 private:
  std::shared_ptr<impl::PoolImpl> impl_;
};

using PoolPtr = std::shared_ptr<Pool>;

}  // namespace storages::mongo

USERVER_NAMESPACE_END
