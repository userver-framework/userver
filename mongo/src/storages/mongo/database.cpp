#include <storages/mongo/database.hpp>

#include <memory>
#include <string>
#include <string_view>

#include <mongoc/mongoc.h>

#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/mongo_error.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/text.hpp>

#include <formats/bson/wrappers.hpp>
#include <storages/mongo/cdriver/collection_impl.hpp>
#include <storages/mongo/cdriver/cursor_impl.hpp>
#include <storages/mongo/cdriver/pool_impl.hpp>
#include <storages/mongo/cdriver/request_helpers.hpp>
#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/operations_common.hpp>
#include <storages/mongo/operations_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {
namespace {

// Database-level commands are accounted under MongoDB's reserved `$cmd` collection.
constexpr std::string_view kDatabaseStatsCollection = "$cmd";

cdriver::DatabasePtr GetNativeDatabase(mongoc_client_t* client, const std::string& name) {
    return cdriver::DatabasePtr(mongoc_client_get_database(client, name.c_str()));
}

}  // namespace

Database::Database(PoolImplPtr pool, std::string database_name)
    : pool_(std::move(pool)),
      database_name_(std::move(database_name))
{
    if (!utils::text::IsCString(database_name_)) {
        throw MongoException("Invalid database name: '" + database_name_);
    }
}

void Database::DropDatabase() {
    auto client = cdriver::GetCDriverPool(pool_).Acquire();
    const auto database = GetNativeDatabase(client.get(), database_name_);

    MongoError error;
    mongoc_database_drop(database.get(), error.GetNative());
    if (error) {
        error.Throw("Error dropping the database");
    }
}

bool Database::HasCollection(utils::zstring_view collection_name) const {
    if (!utils::text::IsCString(collection_name)) {
        throw MongoException(utils::StrCat("Invalid collection name: '", collection_name, "\'"));
    }

    auto client = cdriver::GetCDriverPool(pool_).Acquire();
    const auto database = GetNativeDatabase(client.get(), database_name_);

    MongoError error;
    const bool
        has_collection = mongoc_database_has_collection(database.get(), collection_name.c_str(), error.GetNative());
    if (error) {
        error.Throw("Error checking for collection existence");
    }
    return has_collection;
}

Collection Database::GetCollection(std::string collection_name) const {
    return Collection(std::make_shared<
                      cdriver::CDriverCollectionImpl>(pool_, database_name_, std::move(collection_name)));
}

std::vector<std::string> Database::ListCollectionNames() const {
    auto client = cdriver::GetCDriverPool(pool_).Acquire();
    const auto database = GetNativeDatabase(client.get(), database_name_);

    MongoError error;
    const formats::bson::impl::RawPtr<char*>
        collection_names(mongoc_database_get_collection_names_with_opts(database.get(), nullptr, error.GetNative()));
    if (error) {
        error.Throw("Error listing existing collections");
    }

    auto raw_collection_names = collection_names.get();
    UASSERT(raw_collection_names);

    std::vector<std::string> collections;
    while (*raw_collection_names) {
        const formats::bson::impl::RawPtr<char> collection_name(*raw_collection_names);
        collections.emplace_back(collection_name.get());
        ++raw_collection_names;
    }

    return collections;
}

cdriver::DatabaseRequestContext Database::MakeRequestContext(
    std::string&& span_name,
    const stats::OperationKey& stats_key
) const {
    tracing::Span span(std::move(span_name));
    span.AddTag(tracing::kDatabaseType, tracing::kDatabaseMongoType);
    span.AddTag(tracing::kDatabaseInstance, database_name_);

    auto& pool = cdriver::GetCDriverPool(pool_);
    auto collection_stats = pool_->GetStatistics().collections[std::string{kDatabaseStatsCollection}];
    auto base = cdriver::MakeRequestContextBase(
        std::move(span),
        collection_stats->items[stats_key],
        pool_->GetConfig(),
        [&pool](stats::OperationStatisticsItem& stats) { return cdriver::AcquireClient(pool, stats); }
    );
    auto database = GetNativeDatabase(base.client.get(), database_name_);
    return cdriver::DatabaseRequestContext{std::move(base), std::move(database)};
}

template <typename Operation>
cdriver::DatabaseRequestContext Database::MakeRequestContext(std::string&& span_name, const Operation& operation)
    const {
    return MakeRequestContext(std::move(span_name), operation.impl_->op_key);
}

Cursor Database::Aggregate(const operations::Aggregate& operation) {
    auto context = MakeRequestContext("mongo_aggregate", operation);

    auto options = operation.impl_->options;
    cdriver::PrepareCursorOptions(
        options,
        operation.impl_->max_server_time,
        operation.impl_->has_comment_option,
        context
    );

    auto& pool = cdriver::GetCDriverPool(pool_);
    const auto read_prefs =
        cdriver::MakeReadPrefsWithDefaultMaxStaleness(operation.impl_->read_prefs, pool.GetMaxReplicationLag());
    auto pipeline_doc = operation.impl_->pipeline.GetInternalArrayDocument();
    const bson_t* native_pipeline_bson_ptr = pipeline_doc.GetBson().get();
    cdriver::CursorPtr cdriver_cursor(mongoc_database_aggregate(
        context.database.get(),
        native_pipeline_bson_ptr,
        GetNative(options),
        read_prefs.Get()
    ));
    return Cursor(std::make_unique<cdriver::CDriverCursorImpl>(
        std::move(context.client),
        std::move(cdriver_cursor),
        std::move(context.stats)
    ));
}

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
