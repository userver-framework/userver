#pragma once

#include <string>
#include <vector>

#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/cursor.hpp>
#include <userver/storages/mongo/operations.hpp>
#include <userver/utils/zstring_view.hpp>

#include <storages/mongo/cdriver/request_helpers.hpp>
#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/pool_impl.hpp>
#include <storages/mongo/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

namespace cdriver {

struct DatabaseRequestContext : RequestContextBase {
    DatabasePtr database;
};

}  // namespace cdriver

class Database {
public:
    Database(PoolImplPtr pool, std::string database_name);

    void DropDatabase();

    bool HasCollection(utils::zstring_view collection_name) const;

    Collection GetCollection(std::string collection_name) const;

    std::vector<std::string> ListCollectionNames() const;

    Cursor Aggregate(const operations::Aggregate& operation);

private:
    cdriver::DatabaseRequestContext MakeRequestContext(std::string&& span_name, const stats::OperationKey& stats_key)
        const;

    template <typename Operation>
    cdriver::DatabaseRequestContext MakeRequestContext(std::string&& span_name, const Operation& operation) const;

    PoolImplPtr pool_;
    std::string database_name_;
};

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
