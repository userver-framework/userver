#pragma once

/// @file userver/storages/mongo/component.hpp
/// @brief @copybrief components::Mongo

#include <userver/components/component_base.hpp>
#include <userver/storages/mongo/multi_mongo.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief MongoDB client component
///
/// Provides access to a MongoDB database.
///
/// ## Dynamic options:
/// * @ref MONGO_CONGESTION_CONTROL_DATABASES_SETTINGS
/// * @ref MONGO_CONGESTION_CONTROL_ENABLED
/// * @ref MONGO_CONGESTION_CONTROL_SETTINGS
/// * @ref MONGO_CONNECTION_POOL_SETTINGS
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
/// ## Static options of components::Mongo :
/// @include{doc} scripts/docs/en/components_schema/mongo/src/storages/mongo/component.md
///
/// Options inherited from @ref components::MultiMongo
/// @include{doc} scripts/docs/en/components_schema/mongo/src/storages/mongo/component_multi.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// `stats_verbosity` accepts one of the following values:
/// Value | Description
/// ----- | -----------
/// terse | Default value, report only cumulative stats and read/write totals
/// full | Separate metrics for each operation, divided by read preference or write concern
///
/// It is a common practice to provide a database connection string via
/// environment variables. To retrieve a value from the environment use
/// `dbconnection#env: THE_ENV_VARIABLE_WITH_CONNECTION_STRING` as described
/// in yaml_config::YamlConfig.
///
/// Note that if the `dbalias` option is provided and the components::Secdist component has `update-period` other
/// than 0, then new connections are created or gracefully closed as the secdist configuration change to new value.
///
/// ## Secdist format
///
/// If a `dbalias` option is provided, for example
/// `dbalias: some_name_of_your_database`, then the Secdist entry for that alias
/// should look like following:
/// @code{.json}
/// {
///   "mongo_settings": {
///     "some_name_of_your_database": {
///       "uri": "mongodb://user:password@host:port/database_name"
///     }
///   }
/// }
/// @endcode
class Mongo : public ComponentBase {
public:
    /// Component constructor
    Mongo(const ComponentConfig&, const ComponentContext&);

    /// Component destructor
    ~Mongo() override;

    /// Client pool accessor
    storages::mongo::PoolPtr GetPool() const;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    void OnSecdistUpdate(const storages::secdist::SecdistConfig& config);

    std::string dbalias_;
    storages::mongo::PoolPtr pool_;

    // Subscriptions must be the last fields.
    concurrent::AsyncEventSubscriberScope secdist_subscriber_;
    utils::statistics::Entry statistics_holder_;
};

template <>
inline constexpr bool kHasValidate<Mongo> = true;

/// @ingroup userver_components
///
/// @brief Dynamically configurable MongoDB client component
///
/// Provides access to a dynamically reconfigurable set of MongoDB databases.
///
/// ## Dynamic options:
/// * @ref MONGO_CONGESTION_CONTROL_DATABASES_SETTINGS
/// * @ref MONGO_CONGESTION_CONTROL_ENABLED
/// * @ref MONGO_CONGESTION_CONTROL_SETTINGS
/// * @ref MONGO_CONNECTION_POOL_SETTINGS
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
/// @include{doc} scripts/docs/en/components_schema/mongo/src/storages/mongo/component_multi.md
///
/// `stats_verbosity` accepts one of the following values:
/// Value | Description
/// ----- | -----------
/// terse | Default value, report only cumulative stats and read/write totals
/// full | Separate metrics for each operation, divided by read preference or write concern
///
/// Note that if the components::Secdist component has `update-period` other
/// than 0, then new connections are created or gracefully closed as the secdist configuration change to new value.
class MultiMongo : public ComponentBase {
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

    // Subscriptions must be the last fields.
    utils::statistics::Entry statistics_holder_;
};

template <>
inline constexpr bool kHasValidate<MultiMongo> = true;

}  // namespace components

USERVER_NAMESPACE_END
