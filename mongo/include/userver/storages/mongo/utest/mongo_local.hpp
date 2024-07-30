#pragma once

/// @file userver/storages/mongo/utest/mongo_local.hpp
/// @brief @copybrief storages::mongo::MongoLocal

#include <userver/storages/mongo/pool.hpp>

#include <userver/dynamic_config/test_helpers.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::utest {

/// @brief Make default pool config for mongo local
/// @note This configuration should be used for testing purposes
storages::mongo::PoolConfig MakeDefaultPoolConfig();

/// @brief Mongo local class
class MongoLocal {
 public:
  /// Mongo local default constructor (use default params)
  MongoLocal();

  /// Mongo local constructor with specified params
  MongoLocal(std::optional<std::string_view> dbname,
             std::optional<storages::mongo::PoolConfig> pool_config,
             dynamic_config::Source config_source =
                 dynamic_config::GetDefaultSource());

  /// Get pool
  storages::mongo::PoolPtr GetPool() const;

 private:
  storages::mongo::PoolPtr pool_;
};

}  // namespace storages::mongo::utest

USERVER_NAMESPACE_END
