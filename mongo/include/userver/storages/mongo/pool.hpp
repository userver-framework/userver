#pragma once

/// @file userver/storages/mongo/pool.hpp
/// @brief @copybrief storages::mongo::Pool

#include <memory>
#include <string>
#include <vector>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/dynamic_config/fwd.hpp>
#include <userver/formats/bson/value.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/cursor.hpp>
#include <userver/storages/mongo/operations.hpp>
#include <userver/storages/mongo/pool_config.hpp>
#include <userver/storages/mongo/transaction.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/zstring_view.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

namespace impl {
class PoolImpl;
}  // namespace impl

/// @ingroup userver_clients
///
/// @brief MongoDB client pool.
///
/// Use constructor only for tests, in production the pool should be retrieved
/// from @ref userver_components "the components" via
/// components::Mongo::GetPool() or components::MultiMongo::GetPool().
///
/// ## Example usage:
///
/// @snippet mongo/src/storages/mongo/collection_mongotest.hpp  Sample Mongo usage
class Pool {
public:
    Pool(Pool&&) noexcept;
    Pool& operator=(Pool&&) noexcept;
    ~Pool();

    /// Checks whether a collection exists
    bool HasCollection(utils::zstring_view name) const;

    /// Returns a handle for the specified collection
    Collection GetCollection(std::string name) const;

    /// Drops the associated database if it exists. New modifications of
    /// collections will attempt to re-create the database automatically.
    void DropDatabase();

    /// Get a list of all the collection names in the associated database
    std::vector<std::string> ListCollectionNames() const;

    /// @throws storages::mongo::MongoException if failed to connect to the mongo server.
    void Ping();

    /// @brief Begin a new transaction.
    ///
    /// @return Transaction handle for executing operations within transaction context
    /// @throws MongoException if transaction cannot be started
    Transaction BeginTransaction() const;

    /// @brief Executes an aggregation pipeline on the database, without a collection
    /// @param pipeline an array of aggregation operations
    /// @param options see @ref storages::mongo::options
    ///
    /// Corresponds to MongoDB `db.aggregate([...])`. Use this for pipelines that
    /// cannot run on a collection, for example when the first stage is `$documents`.
    ///
    /// On sharded clusters MongoDB may reject `$documents` together with `$lookup`:
    /// `$documents` must run on mongos, while `$lookup` must run on a shard.
    /// @see Collection::Aggregate
    /// @snippet storages/mongo/pool_mongotest.cpp Sample Mongo database aggregate
    template <typename... Options>
    Cursor Aggregate(formats::bson::Value pipeline, Options&&... options);

    /// @name Prepared operation executors
    /// @{
    Cursor Execute(const operations::Aggregate&);
    /// @}

    /// @cond
    // For internal use only
    Pool(
        std::string id,
        const std::string& uri,
        const PoolConfig& pool_config,
        clients::dns::Resolver* dns_resolver,
        dynamic_config::Source config_source
    );

    // Writes pool statistics
    friend void DumpMetric(utils::statistics::Writer& writer, const Pool& pool);

    // Sets new dynamic pool settings
    void SetPoolSettings(const PoolSettings& pool_settings);

    void SetConnectionString(const std::string& connection_string);
    /// @endcond

private:
    std::shared_ptr<impl::PoolImpl> impl_;
};

using PoolPtr = std::shared_ptr<Pool>;

template <typename... Options>
Cursor Pool::Aggregate(formats::bson::Value pipeline, Options&&... options) {
    operations::Aggregate aggregate(std::move(pipeline));
    (aggregate.SetOption(std::forward<Options>(options)), ...);
    return Execute(aggregate);
}

}  // namespace storages::mongo

USERVER_NAMESPACE_END
