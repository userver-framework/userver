#pragma once

/// @file userver/storages/mongo/component.hpp
/// @brief @copybrief components::Mongo

#include <userver/components/loggable_component_base.hpp>
#include <userver/storages/mongo/multi_mongo.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief MongoDB client component
///
/// Provides access to a MongoDB database.
///
/// ## Dynamic options:
/// * @ref MONGO_DEFAULT_MAX_TIME_MS
///
/// ## Static configuration example:
///
/// ```
/// mongo-taxi:
///   dbalias: taxi
///   appname: userver-sample
///   conn_timeout: 2s
///   so_timeout: 10s
///   queue_timeout: 1s
///   initial_size: 16
///   max_size: 128
///   idle_limit: 64
///   connecting_limit: 8
///   local_threshold: 15ms
///   maintenance_period: 15s
///   stats_verbosity: terse
/// ```
/// You must specify one of `dbalias` or `dbconnection`.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// dbalias | name of the database in secdist config (if available) | --
/// dbconnection | connection string (used if no dbalias specified) | --
/// appname | application name for the DB server | userver
/// conn_timeout | connection timeout | 2s
/// so_timeout | socket timeout | 10s
/// queue_timeout | max connection queue wait time | 1s
/// initial_size | number of connections created initially | 16
/// max_size | limit for total connections number | 128
/// idle_limit | limit for idle connections number | 64
/// connecting_limit | limit for establishing connections number | 8
/// local_threshold | latency window for instance selection | mongodb default
/// max_replication_lag | replication lag limit for usable secondaries, min. 90s | -
/// maintenance_period | pool maintenance period (idle connections pruning etc.) | 15s
/// stats_verbosity | changes the granularity of reported metrics | 'terse'
/// dns_resolver | server hostname resolver type (getaddrinfo or async) | 'async'
///
/// `stats_verbosity` accepts one of the following values:
/// Value | Description
/// ----- | -----------
/// terse | Default value, report only cumulative stats and read/write totals
/// full | Separate metrics for each operation, divided by read preference or write concern

// clang-format on

class Mongo : public LoggableComponentBase {
 public:
  /// Component constructor
  Mongo(const ComponentConfig&, const ComponentContext&);

  /// Component destructor
  ~Mongo() override;

  /// Client pool accessor
  storages::mongo::PoolPtr GetPool() const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  storages::mongo::PoolPtr pool_;
  utils::statistics::Entry statistics_holder_;
};

template <>
inline constexpr bool kHasValidate<Mongo> = true;

// clang-format off

/// @ingroup userver_components
///
/// @brief Dynamically configurable MongoDB client component
///
/// Provides access to a dynamically reconfigurable set of MongoDB databases.
///
/// ## Dynamic options:
/// * @ref MONGO_DEFAULT_MAX_TIME_MS
///
/// ## Static configuration example:
///
/// ```
/// multi-mongo:
///   appname: userver-sample
///   conn_timeout: 2s
///   so_timeout: 10s
///   queue_timeout: 1s
///   initial_size: 16
///   max_size: 128
///   idle_limit: 64
///   connecting_limit: 8
///   local_threshold: 15ms
///   stats_verbosity: terse
/// ```
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// appname | application name for the DB server | userver
/// conn_timeout | connection timeout | 2s
/// so_timeout | socket timeout | 10s
/// queue_timeout | max connection queue wait time | 1s
/// initial_size | number of connections created initially (per database) | 16
/// max_size | limit for total connections number (per database) | 128
/// idle_limit | limit for idle connections number (per database) | 64
/// connecting_limit | limit for establishing connections number (per database) | 8
/// local_threshold | latency window for instance selection | mongodb default
/// max_replication_lag | replication lag limit for usable secondaries, min. 90s | -
/// stats_verbosity | changes the granularity of reported metrics | 'terse'
/// dns_resolver | server hostname resolver type (getaddrinfo or async) | 'async'
///
/// `stats_verbosity` accepts one of the following values:
/// Value | Description
/// ----- | -----------
/// terse | Default value, report only cumulative stats and read/write totals
/// full | Separate metrics for each operation, divided by read preference or write concern

// clang-format on

class MultiMongo : public LoggableComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of components::MultiMongo
  static constexpr std::string_view kName = "multi-mongo";

  /// Component constructor
  MultiMongo(const ComponentConfig&, const ComponentContext&);

  /// Component destructor
  ~MultiMongo() override;

  /// @brief Client pool accessor
  /// @param dbalias name previously passed to `AddPool`
  /// @throws PoolNotFound if no such database is enabled
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

  /// Creates an empty database set bound to the component
  storages::mongo::MultiMongo::PoolSet NewPoolSet();

  using PoolSet = storages::mongo::MultiMongo::PoolSet;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  storages::mongo::MultiMongo multi_mongo_;
  utils::statistics::Entry statistics_holder_;
};

template <>
inline constexpr bool kHasValidate<MultiMongo> = true;

}  // namespace components

USERVER_NAMESPACE_END
