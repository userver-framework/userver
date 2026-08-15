#include <storages/mongo/cdriver/collection_impl.hpp>

#include <type_traits>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/mongo_error.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/utils/text.hpp>

#include <formats/bson/wrappers.hpp>
#include <storages/mongo/cdriver/cursor_impl.hpp>
#include <storages/mongo/cdriver/find_and_modify.hpp>
#include <storages/mongo/cdriver/pool_impl.hpp>
#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/operations_common.hpp>
#include <storages/mongo/operations_impl.hpp>

#include <dynamic_config/variables/MONGO_DEFAULT_MAX_TIME_MS.hpp>
#include <dynamic_config/variables/USERVER_DEADLINE_PROPAGATION_ENABLED.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {
namespace {

const std::string kCancelledByDeadlineTag = "cancelled_by_deadline";
const std::string kCancelledTag = "cancelled";
const std::string kMaxTimeMsTag = "max_time_ms";

class WriteResultHelper {
public:
    bson_t* GetNative() { return bson_.Get(); }
    MongoError& GetError() { return error_; }

    WriteResult Extract() { return WriteResult(formats::bson::Document(bson_.Extract()), std::move(error_)); }

private:
    formats::bson::impl::UninitializedBson bson_;
    MongoError error_{};
};

std::optional<std::string_view> GetCurrentSpanLink() {
    auto* span = tracing::Span::CurrentSpanUnchecked();
    if (span) {
        return span->GetLink();
    }
    return std::nullopt;
}

void SetLinkComment(formats::bson::impl::BsonBuilder& builder, bool& has_comment_option) {
    auto link = GetCurrentSpanLink();
    if (link) {
        operations::AppendComment(builder, has_comment_option, options::Comment(utils::StrCat("link=", *link)));
    }
}

std::optional<std::chrono::milliseconds> GetDeadlineTimeLeft(const dynamic_config::Snapshot& config) {
    if (!config[::dynamic_config::USERVER_DEADLINE_PROPAGATION_ENABLED]) {
        return std::nullopt;
    }

    const auto inherited_deadline = server::request::GetTaskInheritedDeadline();
    if (!inherited_deadline.IsReachable()) {
        return std::nullopt;
    }

    const auto
        inherited_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(inherited_deadline.TimeLeftApprox());
    return inherited_timeout;
}

std::chrono::milliseconds ComputeAdjustedMaxServerTime(
    std::chrono::milliseconds user_max_server_time,
    const RequestContext& context
) {
    auto max_server_time = user_max_server_time;
    try {
        operations::VerifyMaxServerTime(max_server_time);
    } catch (const InvalidQueryArgumentException& /*ex*/) {
        context.stats->Account(stats::ErrorType::kBadQueryArgument);
        throw;
    }

    if (max_server_time == operations::kNoMaxServerTime) {
        max_server_time = context.dynamic_config[::dynamic_config::MONGO_DEFAULT_MAX_TIME_MS];
    }

    if (auto inherited_deadline = context.inherited_deadline) {
        operations::VerifyMaxServerTime(*inherited_deadline);
        if (max_server_time == operations::kNoMaxServerTime || *inherited_deadline < max_server_time) {
            max_server_time = *inherited_deadline;
        }
    }

    if (max_server_time != operations::kNoMaxServerTime) {
        tracing::Span::CurrentSpan().AddTag(kMaxTimeMsTag, max_server_time.count());
    }

    return max_server_time;
}

void SetMaxServerTime(
    std::optional<formats::bson::impl::BsonBuilder>& builder,
    std::chrono::milliseconds max_server_time,
    const RequestContext& context
) {
    max_server_time = ComputeAdjustedMaxServerTime(max_server_time, context);
    if (max_server_time == operations::kNoMaxServerTime) {
        return;
    }

    constexpr std::string_view kOptionName = "maxTimeMS";
    impl::EnsureBuilder(builder).Append(kOptionName, max_server_time.count());
}

void SetMaxServerTime(
    mongoc_find_and_modify_opts_t& options,
    std::chrono::milliseconds max_server_time,
    const RequestContext& context
) {
    max_server_time = ComputeAdjustedMaxServerTime(max_server_time, context);
    if (max_server_time == operations::kNoMaxServerTime) {
        return;
    }

    if (!mongoc_find_and_modify_opts_set_max_time_ms(&options, max_server_time.count())) {
        throw MongoException("Cannot set max server time");
    }
}

#ifdef MONGOC_BULKWRITE_H

void SetUpsert(mongoc_bulkwrite_updateoneopts_t* opts, bool upsert) {
    mongoc_bulkwrite_updateoneopts_set_upsert(opts, upsert);
}

void SetUpsert(mongoc_bulkwrite_updatemanyopts_t* opts, bool upsert) {
    mongoc_bulkwrite_updatemanyopts_set_upsert(opts, upsert);
}

void SetUpsert(mongoc_bulkwrite_replaceoneopts_t* opts, bool upsert) {
    mongoc_bulkwrite_replaceoneopts_set_upsert(opts, upsert);
}

void SetHint(mongoc_bulkwrite_updateoneopts_t* opts, const bson_value_t* hint) {
    mongoc_bulkwrite_updateoneopts_set_hint(opts, hint);
}

void SetHint(mongoc_bulkwrite_updatemanyopts_t* opts, const bson_value_t* hint) {
    mongoc_bulkwrite_updatemanyopts_set_hint(opts, hint);
}

void SetHint(mongoc_bulkwrite_replaceoneopts_t* opts, const bson_value_t* hint) {
    mongoc_bulkwrite_replaceoneopts_set_hint(opts, hint);
}

void SetCollation(mongoc_bulkwrite_updateoneopts_t* opts, const bson_t* collation) {
    mongoc_bulkwrite_updateoneopts_set_collation(opts, collation);
}

void SetCollation(mongoc_bulkwrite_updatemanyopts_t* opts, const bson_t* collation) {
    mongoc_bulkwrite_updatemanyopts_set_collation(opts, collation);
}

void SetCollation(mongoc_bulkwrite_replaceoneopts_t* opts, const bson_t* collation) {
    mongoc_bulkwrite_replaceoneopts_set_collation(opts, collation);
}

void SetArrayFilters(mongoc_bulkwrite_updateoneopts_t* opts, const bson_t* array_filters) {
    mongoc_bulkwrite_updateoneopts_set_arrayfilters(opts, array_filters);
}

void SetArrayFilters(mongoc_bulkwrite_updatemanyopts_t* opts, const bson_t* array_filters) {
    mongoc_bulkwrite_updatemanyopts_set_arrayfilters(opts, array_filters);
}

void SetSort(mongoc_bulkwrite_updateoneopts_t* opts, const bson_t* sort) {
    mongoc_bulkwrite_updateoneopts_set_sort(opts, sort);
}

void SetSort(mongoc_bulkwrite_replaceoneopts_t* opts, const bson_t* sort) {
    mongoc_bulkwrite_replaceoneopts_set_sort(opts, sort);
}

void InitBsonView(const bson_iter_t& iter, bson_t& view) {
    std::uint32_t length = 0;
    const std::uint8_t* data = nullptr;
    if (BSON_ITER_HOLDS_DOCUMENT(&iter)) {
        bson_iter_document(&iter, &length, &data);
    } else if (BSON_ITER_HOLDS_ARRAY(&iter)) {
        bson_iter_array(&iter, &length, &data);
    } else {
        throw MongoException("Unexpected BSON type of option '") << bson_iter_key(&iter) << '\'';
    }
    if (!bson_init_static(&view, data, length)) {
        throw MongoException("Invalid BSON of option '") << bson_iter_key(&iter) << '\'';
    }
}

WriteConcernPtr BuildWriteConcernFromBson(const bson_iter_t& write_concern_iter) {
    bson_iter_t iter;
    if (!BSON_ITER_HOLDS_DOCUMENT(&write_concern_iter) || !bson_iter_recurse(&write_concern_iter, &iter)) {
        throw MongoException("Invalid 'writeConcern' option");
    }

    WriteConcernPtr write_concern(mongoc_write_concern_new());
    while (bson_iter_next(&iter)) {
        const std::string_view key{bson_iter_key(&iter), bson_iter_key_len(&iter)};
        if (key == "w") {
            if (BSON_ITER_HOLDS_UTF8(&iter)) {
                std::uint32_t length = 0;
                const char* value = bson_iter_utf8(&iter, &length);
                if (std::string_view{value, length} == "majority") {
                    mongoc_write_concern_set_wmajority(write_concern.get(), -1);
                } else {
                    mongoc_write_concern_set_wtag(write_concern.get(), value);
                }
            } else if (BSON_ITER_HOLDS_INT32(&iter)) {
                mongoc_write_concern_set_w(write_concern.get(), bson_iter_int32(&iter));
            } else {
                throw MongoException("Unexpected BSON type of write concern field 'w'");
            }
        } else if (key == "j") {
            mongoc_write_concern_set_journal(write_concern.get(), bson_iter_bool(&iter));
        } else if (key == "wtimeout") {
            mongoc_write_concern_set_wtimeout_int64(write_concern.get(), bson_iter_as_int64(&iter));
        } else {
            throw MongoException("Unexpected write concern field '") << key << '\'';
        }
    }
    return write_concern;
}

bool IsWriteAcknowledged(
    const std::optional<formats::bson::impl::BsonBuilder>& options,
    const mongoc_collection_t* collection
) {
    const mongoc_write_concern_t* write_concern = mongoc_collection_get_write_concern(collection);
    WriteConcernPtr operation_write_concern;

    const bson_t* options_bson = impl::GetNative(options);
    bson_iter_t iter;
    if (options_bson && bson_iter_init_find(&iter, options_bson, "writeConcern")) {
        operation_write_concern = BuildWriteConcernFromBson(iter);
        write_concern = operation_write_concern.get();
    }

    UASSERT(write_concern);
    return mongoc_write_concern_is_acknowledged(write_concern);
}

bool ApplyBulkWriteCommandOption(bson_iter_t& iter, std::string_view key, mongoc_bulkwriteopts_t& bulk_opts) {
    if (key == "sessionId") {
        // The session is applied through mongoc_bulkwrite_set_session.
        return true;
    }

    if (key == "writeConcern") {
        const auto write_concern = BuildWriteConcernFromBson(iter);
        mongoc_bulkwriteopts_set_writeconcern(&bulk_opts, write_concern.get());
    } else if (key == "comment") {
        mongoc_bulkwriteopts_set_comment(&bulk_opts, bson_iter_value(&iter));
    } else if (key == "let") {
        bson_t let_view;
        InitBsonView(iter, let_view);
        mongoc_bulkwriteopts_set_let(&bulk_opts, &let_view);
    } else if (key == "bypassDocumentValidation") {
        mongoc_bulkwriteopts_set_bypassdocumentvalidation(&bulk_opts, bson_iter_bool(&iter));
    } else {
        return false;
    }
    return true;
}

template <typename StatementOptsPtr>
bool ApplyBulkWriteStatementOption(bson_iter_t& iter, std::string_view key, StatementOptsPtr& statement_opts) {
    if (key == "upsert") {
        SetUpsert(statement_opts.get(), bson_iter_bool(&iter));
    } else if (key == "hint") {
        SetHint(statement_opts.get(), bson_iter_value(&iter));
    } else if (key == "collation") {
        bson_t collation_view;
        InitBsonView(iter, collation_view);
        SetCollation(statement_opts.get(), &collation_view);
    } else if (key == "arrayFilters") {
        if constexpr (std::is_same_v<StatementOptsPtr, ReplaceOneOptsPtr>) {
            throw MongoException("'arrayFilters' is not supported by replace operations");
        } else {
            bson_t array_filters_view;
            InitBsonView(iter, array_filters_view);
            SetArrayFilters(statement_opts.get(), &array_filters_view);
        }
    } else if (key == "sort") {
        if constexpr (std::is_same_v<StatementOptsPtr, UpdateManyOptsPtr>) {
            throw MongoException("'sort' is not supported by multi-document updates");
        } else {
            bson_t sort_view;
            InitBsonView(iter, sort_view);
            SetSort(statement_opts.get(), &sort_view);
        }
    } else {
        return false;
    }
    return true;
}

template <typename StatementOptsPtr>
StatementOptsPtr MakeBulkWriteStatementOpts(
    StatementOptsPtr statement_opts,
    const std::optional<formats::bson::impl::BsonBuilder>& options,
    mongoc_bulkwriteopts_t& bulk_opts
) {
    const bson_t* options_bson = impl::GetNative(options);
    if (!options_bson) {
        return statement_opts;
    }

    bson_iter_t iter;
    if (!bson_iter_init(&iter, options_bson)) {
        throw MongoException("Invalid write operation options");
    }
    while (bson_iter_next(&iter)) {
        const std::string_view key{bson_iter_key(&iter), bson_iter_key_len(&iter)};
        if (ApplyBulkWriteCommandOption(iter, key, bulk_opts) ||
            ApplyBulkWriteStatementOption(iter, key, statement_opts))
        {
            continue;
        }

        throw MongoException("Unexpected write operation option '") << key << '\'';
    }
    return statement_opts;
}

[[maybe_unused]] UpdateOneOptsPtr MakeUpdateOneOpts(
    const std::optional<formats::bson::impl::BsonBuilder>& options,
    mongoc_bulkwriteopts_t& bulk_opts
) {
    return MakeBulkWriteStatementOpts(UpdateOneOptsPtr{mongoc_bulkwrite_updateoneopts_new()}, options, bulk_opts);
}

[[maybe_unused]] UpdateManyOptsPtr MakeUpdateManyOpts(
    const std::optional<formats::bson::impl::BsonBuilder>& options,
    mongoc_bulkwriteopts_t& bulk_opts
) {
    return MakeBulkWriteStatementOpts(UpdateManyOptsPtr{mongoc_bulkwrite_updatemanyopts_new()}, options, bulk_opts);
}

ReplaceOneOptsPtr MakeReplaceOneOpts(
    const std::optional<formats::bson::impl::BsonBuilder>& options,
    mongoc_bulkwriteopts_t& bulk_opts
) {
    return MakeBulkWriteStatementOpts(ReplaceOneOptsPtr{mongoc_bulkwrite_replaceoneopts_new()}, options, bulk_opts);
}

constexpr std::int32_t kDuplicateKeyErrorCode = 11000;

constexpr std::uint32_t kCommandNotFoundErrorCode = 59;

bool IsBulkWriteUnsupportedError(const MongoError& error) { return error.Code() == kCommandNotFoundErrorCode; }

std::size_t ParseModelIndex(const bson_iter_t& iter) {
    return static_cast<std::size_t>(bson_ascii_strtoll(bson_iter_key(&iter), nullptr, 10));
}

void AppendBulkWriteUpserted(bson_t* out, const BulkWriteResultPtr& result) {
    if (mongoc_bulkwriteresult_upsertedcount(result.get()) == 0) {
        return;
    }
    const bson_t* update_results = mongoc_bulkwriteresult_updateresults(result.get());
    if (!update_results) {
        return;
    }

    bson_iter_t iter;
    if (!bson_iter_init(&iter, update_results)) {
        return;
    }

    constexpr std::string_view kKey = "upserted";
    formats::bson::impl::SubarrayBson array(out, kKey.data(), kKey.size());
    formats::bson::impl::ArrayIndexer indexer;
    while (bson_iter_next(&iter)) {
        bson_iter_t result_iter;
        if (!BSON_ITER_HOLDS_DOCUMENT(&iter) || !bson_iter_recurse(&iter, &result_iter) ||
            !bson_iter_find(&result_iter, "upsertedId"))
        {
            continue;
        }
        const bson_value_t* upserted_id = bson_iter_value(&result_iter);
        const auto model_index = ParseModelIndex(iter);
        const auto element_key = indexer.GetKey();
        formats::bson::impl::SubdocBson element(array.Get(), element_key.data(), element_key.size());
        indexer.Advance();
        bson_append_int64(element.Get(), "index", -1, static_cast<std::int64_t>(model_index));
        bson_append_value(element.Get(), "_id", -1, upserted_id);
    }
}

void AppendErrorCodeAndMessage(bson_t* element, bson_iter_t& error_iter) {
    while (bson_iter_next(&error_iter)) {
        const std::string_view field{bson_iter_key(&error_iter), bson_iter_key_len(&error_iter)};
        if (field == "code") {
            bson_append_int32(element, "code", -1, static_cast<std::int32_t>(bson_iter_as_int64(&error_iter)));
        } else if (field == "message") {
            std::uint32_t length = 0;
            const char* message = bson_iter_utf8(&error_iter, &length);
            bson_append_utf8(element, "errmsg", -1, message, static_cast<int>(length));
        }
    }
}

void AppendBulkWriteErrors(bson_t* out, const BulkWriteExceptionPtr& exception) {
    const bson_t* write_errors = mongoc_bulkwriteexception_writeerrors(exception.get());
    if (!write_errors || bson_empty(write_errors)) {
        return;
    }

    bson_iter_t iter;
    if (!bson_iter_init(&iter, write_errors)) {
        return;
    }

    constexpr std::string_view kKey = "writeErrors";
    formats::bson::impl::SubarrayBson array(out, kKey.data(), kKey.size());
    formats::bson::impl::ArrayIndexer indexer;
    while (bson_iter_next(&iter)) {
        bson_iter_t error_iter;
        if (!BSON_ITER_HOLDS_DOCUMENT(&iter) || !bson_iter_recurse(&iter, &error_iter)) {
            continue;
        }
        const auto model_index = ParseModelIndex(iter);
        const auto element_key = indexer.GetKey();
        formats::bson::impl::SubdocBson element(array.Get(), element_key.data(), element_key.size());
        indexer.Advance();
        bson_append_int64(element.Get(), "index", -1, static_cast<std::int64_t>(model_index));
        AppendErrorCodeAndMessage(element.Get(), error_iter);
    }
}

void AppendBulkWriteConcernErrors(bson_t* out, const BulkWriteExceptionPtr& exception) {
    const bson_t* wc_errors = mongoc_bulkwriteexception_writeconcernerrors(exception.get());
    if (!wc_errors || bson_empty(wc_errors)) {
        return;
    }

    bson_iter_t iter;
    if (!bson_iter_init(&iter, wc_errors)) {
        return;
    }

    constexpr std::string_view kKey = "writeConcernErrors";
    formats::bson::impl::SubarrayBson array(out, kKey.data(), kKey.size());
    formats::bson::impl::ArrayIndexer indexer;
    while (bson_iter_next(&iter)) {
        bson_iter_t error_iter;
        if (!BSON_ITER_HOLDS_DOCUMENT(&iter) || !bson_iter_recurse(&iter, &error_iter)) {
            continue;
        }
        const auto element_key = indexer.GetKey();
        formats::bson::impl::SubdocBson element(array.Get(), element_key.data(), element_key.size());
        indexer.Advance();
        AppendErrorCodeAndMessage(element.Get(), error_iter);
    }
}

MongoError GetBulkWriteError(const BulkWriteExceptionPtr& exception) {
    MongoError error;
    if (!exception) {
        return error;
    }
    if (mongoc_bulkwriteexception_error(exception.get(), error.GetNative())) {
        return error;
    }

    const auto set_first_error = [&error](const bson_t* errors, std::uint32_t domain) {
        if (!errors) {
            return false;
        }
        bson_iter_t iter;
        if (!bson_iter_init(&iter, errors) || !bson_iter_next(&iter)) {
            return false;
        }
        bson_iter_t error_iter;
        if (!BSON_ITER_HOLDS_DOCUMENT(&iter) || !bson_iter_recurse(&iter, &error_iter)) {
            return false;
        }
        std::int64_t code = 0;
        const char* message = "";
        while (bson_iter_next(&error_iter)) {
            const std::string_view field{bson_iter_key(&error_iter), bson_iter_key_len(&error_iter)};
            if (field == "code") {
                code = bson_iter_as_int64(&error_iter);
            } else if (field == "message" && BSON_ITER_HOLDS_UTF8(&error_iter)) {
                message = bson_iter_utf8(&error_iter, nullptr);
            }
        }
        bson_set_error(error.GetNative(), domain, static_cast<std::uint32_t>(code), "%s", message);
        return true;
    };

    if (set_first_error(mongoc_bulkwriteexception_writeerrors(exception.get()), MONGOC_ERROR_SERVER)) {
        return error;
    }
    set_first_error(mongoc_bulkwriteexception_writeconcernerrors(exception.get()), MONGOC_ERROR_WRITE_CONCERN);
    return error;
}

WriteResult FinishBulkWrite(const BulkWriteResultPtr& result, const BulkWriteExceptionPtr& exception) {
    formats::bson::impl::MutableBson document;
    bson_t* out = document.Get();

    if (result) {
        bson_append_int64(out, "insertedCount", -1, mongoc_bulkwriteresult_insertedcount(result.get()));
        bson_append_int64(out, "matchedCount", -1, mongoc_bulkwriteresult_matchedcount(result.get()));
        bson_append_int64(out, "modifiedCount", -1, mongoc_bulkwriteresult_modifiedcount(result.get()));
        bson_append_int64(out, "upsertedCount", -1, mongoc_bulkwriteresult_upsertedcount(result.get()));
        bson_append_int64(out, "deletedCount", -1, mongoc_bulkwriteresult_deletedcount(result.get()));
        AppendBulkWriteUpserted(out, result);
    }

    MongoError error;
    if (exception) {
        AppendBulkWriteErrors(out, exception);
        AppendBulkWriteConcernErrors(out, exception);
        error = GetBulkWriteError(exception);
    }

    return WriteResult(formats::bson::Document(document.Extract()), std::move(error));
}

[[maybe_unused]] bool BulkWriteHasDuplicateKey(const BulkWriteExceptionPtr& exception) {
    if (!exception) {
        return false;
    }
    const bson_t* write_errors = mongoc_bulkwriteexception_writeerrors(exception.get());
    if (!write_errors) {
        return false;
    }
    bson_iter_t iter;
    if (!bson_iter_init(&iter, write_errors)) {
        return false;
    }
    while (bson_iter_next(&iter)) {
        bson_iter_t error_iter;
        if (BSON_ITER_HOLDS_DOCUMENT(&iter) && bson_iter_recurse(&iter, &error_iter) &&
            bson_iter_find(&error_iter, "code") && bson_iter_as_int64(&error_iter) == kDuplicateKeyErrorCode)
        {
            return true;
        }
    }
    return false;
}

#endif  // MONGOC_BULKWRITE_H

std::optional<std::chrono::milliseconds> GetTimeoutOrThrow(
    const dynamic_config::Snapshot& dynamic_config,
    stats::OperationStatisticsItem& stats,
    tracing::Span& span
) {
    const auto time_left = GetDeadlineTimeLeft(dynamic_config);
    if (time_left && time_left <= std::chrono::seconds{0}) {
        stats.Account(stats::ErrorType::kCancelled);
        span.AddTag(kCancelledByDeadlineTag, true);
        throw CancelledException(CancelledException::ByDeadlinePropagation{});
    }
    return time_left;
}

}  // namespace

CDriverCollectionImpl::CDriverCollectionImpl(
    PoolImplPtr pool_impl,
    std::string database_name,
    std::string collection_name
)
    : CollectionImpl(std::move(database_name), std::move(collection_name)),
      pool_impl_(std::move(pool_impl)),
      statistics_(pool_impl_->GetStatistics().collections[GetCollectionName()])
{
    UASSERT(dynamic_cast<cdriver::CDriverPoolImpl*>(pool_impl_.get()));
}

size_t CDriverCollectionImpl::Execute(const operations::Count& operation) const {
    auto context = MakeRequestContext("mongo_count", operation);

    auto options = operation.impl_->options;
    SetMaxServerTime(options, operation.impl_->max_server_time, context);

    MongoError error;
    stats::OperationStopwatch stopwatch(std::move(context.stats));
    const bson_t* native_filter_bson_ptr = operation.impl_->filter.GetBson().get();
    int64_t count = mongoc_collection_count_documents(
        context.collection.get(),
        native_filter_bson_ptr,
        impl::GetNative(operation.impl_->options),
        operation.impl_->read_prefs.Get(),
        nullptr,
        error.GetNative()
    );
    if (count < 0) {
        stopwatch.AccountError(error.GetKind());
        error.Throw("Error counting documents");
    }
    stopwatch.AccountSuccess();
    return count;
}

size_t CDriverCollectionImpl::Execute(const operations::CountApprox& operation) const {
    auto context = MakeRequestContext("mongo_count_approx", operation);

    auto options = operation.impl_->options;
    SetMaxServerTime(options, operation.impl_->max_server_time, context);

    MongoError error;
    stats::OperationStopwatch stopwatch(std::move(context.stats));
    auto count = mongoc_collection_estimated_document_count(
        context.collection.get(),
        impl::GetNative(operation.impl_->options),
        operation.impl_->read_prefs.Get(),
        nullptr,
        error.GetNative()
    );
    if (count < 0) {
        stopwatch.AccountError(error.GetKind());
        error.Throw("Error counting documents");
    }
    stopwatch.AccountSuccess();
    return count;
}

Cursor CDriverCollectionImpl::Execute(const operations::Find& operation) const {
    auto context = MakeRequestContext("mongo_find", operation);

    auto options = operation.impl_->options;
    SetMaxServerTime(options, operation.impl_->max_server_time, context);
    bool has_comment_option = operation.impl_->has_comment_option;
    if (!has_comment_option) {
        SetLinkComment(impl::EnsureBuilder(options), has_comment_option);
    }

    const bson_t* native_filter_bson_ptr = operation.impl_->filter.GetBson().get();
    impl::cdriver::CursorPtr cdriver_cursor(mongoc_collection_find_with_opts(
        context.collection.get(),
        native_filter_bson_ptr,
        impl::GetNative(options),
        operation.impl_->read_prefs.Get()
    ));
    return Cursor(std::make_unique<impl::cdriver::CDriverCursorImpl>(
        std::move(context.client),
        std::move(cdriver_cursor),
        std::move(context.stats)
    ));
}

std::vector<formats::bson::Value> CDriverCollectionImpl::Execute(const operations::Distinct& operation) const {
    auto context = MakeRequestContext("mongo_distinct", operation);

    auto options = operation.impl_->options;
    SetMaxServerTime(options, operation.impl_->max_server_time, context);
    bool has_comment_option = operation.impl_->has_comment_option;
    if (!has_comment_option) {
        SetLinkComment(impl::EnsureBuilder(options), has_comment_option);
    }

    MongoError error;
    stats::OperationStopwatch stopwatch(std::move(context.stats));

    formats::bson::impl::BsonBuilder command_builder;
    command_builder.Append("distinct", GetCollectionName());
    command_builder.Append("key", operation.impl_->field);

    if (operation.impl_->filter.has_value()) {
        command_builder.Append("query", operation.impl_->filter.value());
    }

    auto command_bson = command_builder.Extract();
    const bson_t* native_command_bson_ptr = command_bson.get();

    formats::bson::impl::UninitializedBson result_bson;
    if (!mongoc_collection_read_command_with_opts(
            context.collection.get(),
            native_command_bson_ptr,
            operation.impl_->read_prefs.Get(),
            impl::GetNative(options),
            result_bson.Get(),
            error.GetNative()
        ))
    {
        stopwatch.AccountError(error.GetKind());
        error.Throw("Error executing distinct operation");
    }

    stopwatch.AccountSuccess();

    auto result_doc = formats::bson::Document(result_bson.Extract());
    auto values_field = result_doc["values"];
    if (!values_field.IsArray()) {
        throw MongoException("Distinct operation result is not an array");
    }

    std::vector<formats::bson::Value> distinct_values;
    distinct_values.reserve(values_field.GetSize());

    for (auto& value : values_field) {
        distinct_values.push_back(std::move(value));
    }

    return distinct_values;
}

WriteResult CDriverCollectionImpl::Execute(const operations::InsertOne& operation) {
    auto context = MakeRequestContext("mongo_insert_one", operation);

    auto options = operation.impl_->options;
    SetMaxServerTime(options, operation.impl_->max_server_time, context);

    WriteResultHelper write_result;
    MongoError& error = write_result.GetError();
    stats::OperationStopwatch stopwatch(std::move(context.stats));
    const bson_t* native_bson_ptr = operation.impl_->document.GetBson().get();
    if (mongoc_collection_insert_one(
            context.collection.get(),
            native_bson_ptr,
            impl::GetNative(options),
            write_result.GetNative(),
            error.GetNative()
        ))
    {
        stopwatch.AccountSuccess();
    } else {
        stopwatch.AccountError(error.GetKind());
        if (operation.impl_->should_throw || !error.IsServerError()) {
            error.Throw("Error inserting document");
        }
    }
    return write_result.Extract();
}

WriteResult CDriverCollectionImpl::Execute(const operations::InsertMany& operation) {
    if (operation.impl_->documents.empty()) {
        return {};
    }

    auto context = MakeRequestContext("mongo_insert_many", operation);

    auto options = operation.impl_->options;
    SetMaxServerTime(options, operation.impl_->max_server_time, context);

    // https://jira.mongodb.org/browse/CDRIVER-3378
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
    std::vector<const bson_t*> bsons;
#pragma GCC diagnostic pop
    bsons.reserve(operation.impl_->documents.size());
    for (const auto& doc : operation.impl_->documents) {
        bsons.push_back(doc.GetBson().get());
    }

    WriteResultHelper write_result;
    MongoError& error = write_result.GetError();
    stats::OperationStopwatch stopwatch(std::move(context.stats));
    if (mongoc_collection_insert_many(
            context.collection.get(),
            bsons.data(),
            bsons.size(),
            impl::GetNative(options),
            write_result.GetNative(),
            error.GetNative()
        ))
    {
        stopwatch.AccountSuccess();
    } else {
        stopwatch.AccountError(error.GetKind());
        if (operation.impl_->should_throw || !error.IsServerError()) {
            error.Throw("Error inserting documents");
        }
    }
    return write_result.Extract();
}

WriteResult CDriverCollectionImpl::Execute(const operations::ReplaceOne& operation) {
    auto context = MakeRequestContext("mongo_replace_one", operation);

#ifdef MONGOC_BULKWRITE_H
    const auto effective = ComputeAdjustedMaxServerTime(operation.impl_->max_server_time, context);
    if (effective != operations::kNoMaxServerTime) {
        GetPool().RecheckBulkWriteSupport(context.client.get());
        if (GetPool().IsBulkWriteSupported()) {
            auto write_result = ExecuteReplaceBulkWrite(operation, context, effective);
            if (write_result) {
                return std::move(*write_result);
            }
        }
    }

    if (effective != operations::kNoMaxServerTime) {
        LOG_LIMITED_WARNING()
            << "max_server_time for ReplaceOne is ignored because the MongoDB server does not support the "
               "'bulkWrite' command; MongoDB 8.0 or newer is required";
    }
#endif

    return ExecuteReplaceNative(operation, context);
}

WriteResult CDriverCollectionImpl::ExecuteReplaceNative(
    const operations::ReplaceOne& operation,
    RequestContext& context
) {
    WriteResultHelper write_result;
    MongoError& error = write_result.GetError();
    stats::OperationStopwatch stopwatch(std::move(context.stats));
    const bson_t* native_selector_bson_ptr = operation.impl_->selector.GetBson().get();
    const bson_t* native_replacement_bson_ptr = operation.impl_->replacement.GetBson().get();
    if (mongoc_collection_replace_one(
            context.collection.get(),
            native_selector_bson_ptr,
            native_replacement_bson_ptr,
            impl::GetNative(operation.impl_->options),
            write_result.GetNative(),
            error.GetNative()
        ))
    {
        stopwatch.AccountSuccess();
    } else {
        stopwatch.AccountError(error.GetKind());
        if (operation.impl_->should_throw || !error.IsServerError()) {
            error.Throw("Error replacing document");
        }
    }
    return write_result.Extract();
}

#ifdef MONGOC_BULKWRITE_H

std::optional<WriteResult> CDriverCollectionImpl::ExecuteReplaceBulkWrite(
    const operations::ReplaceOne& operation,
    RequestContext& context,
    std::chrono::milliseconds effective
) {
    const std::string ns = utils::StrCat(GetDatabaseName(), ".", GetCollectionName());
    const bson_t* native_selector_bson_ptr = operation.impl_->selector.GetBson().get();
    const bson_t* native_replacement_bson_ptr = operation.impl_->replacement.GetBson().get();

    auto* const session = GetSession();
    const bool is_in_transaction = session && mongoc_client_session_in_transaction(session);
    const bool is_acknowledged = IsWriteAcknowledged(operation.impl_->options, context.collection.get());

    BulkWritePtr bulk_write{mongoc_client_bulkwrite_new(context.client.get())};
    if (session) {
        mongoc_bulkwrite_set_session(bulk_write.get(), session);
    }

    BulkWriteOptsPtr bulk_opts{mongoc_bulkwriteopts_new()};
    mongoc_bulkwriteopts_set_verboseresults(bulk_opts.get(), is_acknowledged);
    mongoc_bulkwriteopts_set_ordered(bulk_opts.get(), is_acknowledged);

    const auto extra = formats::bson::MakeDoc("maxTimeMS", static_cast<std::int64_t>(effective.count()));
    const bson_t* native_extra_bson_ptr = extra.GetBson().get();
    mongoc_bulkwriteopts_set_extra(bulk_opts.get(), native_extra_bson_ptr);

    MongoError append_error;
    const auto statement_opts = MakeReplaceOneOpts(operation.impl_->options, *bulk_opts);
    if (!mongoc_bulkwrite_append_replaceone(
            bulk_write.get(),
            ns.c_str(),
            native_selector_bson_ptr,
            native_replacement_bson_ptr,
            statement_opts.get(),
            append_error.GetNative()
        ))
    {
        append_error.Throw("Error building replace");
    }

    stats::OperationStopwatch stopwatch(context.stats);
    const mongoc_bulkwritereturn_t ret = mongoc_bulkwrite_execute(bulk_write.get(), bulk_opts.get());
    const BulkWriteResultPtr result{ret.res};
    const BulkWriteExceptionPtr exception{ret.exc};

    auto write_result = FinishBulkWrite(result, exception);
    if (!exception) {
        stopwatch.AccountSuccess();
        return write_result;
    }

    const MongoError& error = write_result.OperationError();
    if (IsBulkWriteUnsupportedError(error)) {
        GetPool().MarkBulkWriteUnsupported();

        if (is_in_transaction) {
            stopwatch.AccountError(error.GetKind());
            error.Throw("Error replacing document");
        }

        stopwatch.Discard();
        return std::nullopt;
    }

    stopwatch.AccountError(error.GetKind());
    if (operation.impl_->should_throw || !error.IsServerError()) {
        error.Throw("Error replacing document");
    }
    return write_result;
}

#endif

WriteResult CDriverCollectionImpl::Execute(const operations::Update& operation) {
    auto context = MakeRequestContext("mongo_update", operation);

    bool should_retry_dupkey = operation.impl_->should_retry_dupkey;
    while (true) {
        WriteResultHelper write_result;
        MongoError& error = write_result.GetError();
        stats::OperationStopwatch stopwatch(context.stats);
        const bson_t* native_selector_bson_ptr = operation.impl_->selector.GetBson().get();
        const bson_t* native_update_bson_ptr = operation.impl_->update.GetBson().get();
        bool has_succeeded = false;
        switch (operation.impl_->mode) {
            case operations::Update::Mode::kSingle:
                has_succeeded = mongoc_collection_update_one(
                    context.collection.get(),
                    native_selector_bson_ptr,
                    native_update_bson_ptr,
                    impl::GetNative(operation.impl_->options),
                    write_result.GetNative(),
                    error.GetNative()
                );
                break;

            case operations::Update::Mode::kMulti:
                has_succeeded = mongoc_collection_update_many(
                    context.collection.get(),
                    native_selector_bson_ptr,
                    native_update_bson_ptr,
                    impl::GetNative(operation.impl_->options),
                    write_result.GetNative(),
                    error.GetNative()
                );
                break;
        }
        if (has_succeeded) {
            stopwatch.AccountSuccess();
        } else {
            auto error_kind = error.GetKind();
            stopwatch.AccountError(error_kind);
            if (should_retry_dupkey && error_kind == MongoError::Kind::kDuplicateKey) {
                UASSERT(operation.impl_->mode == operations::Update::Mode::kSingle);
                should_retry_dupkey = false;
                continue;
            }
            if (operation.impl_->should_throw || !error.IsServerError()) {
                error.Throw("Error updating documents");
            }
        }
        return write_result.Extract();
    }
}

WriteResult CDriverCollectionImpl::Execute(const operations::Delete& operation) {
    auto context = MakeRequestContext("mongo_delete", operation);

    auto options = operation.impl_->options;
    SetMaxServerTime(options, operation.impl_->max_server_time, context);

    WriteResultHelper write_result;
    MongoError& error = write_result.GetError();
    stats::OperationStopwatch stopwatch(std::move(context.stats));
    const bson_t* native_selector_bson_ptr = operation.impl_->selector.GetBson().get();
    bool has_succeeded = false;
    switch (operation.impl_->mode) {
        case operations::Delete::Mode::kSingle:
            has_succeeded = mongoc_collection_delete_one(
                context.collection.get(),
                native_selector_bson_ptr,
                impl::GetNative(options),
                write_result.GetNative(),
                error.GetNative()
            );
            break;

        case operations::Delete::Mode::kMulti:
            has_succeeded = mongoc_collection_delete_many(
                context.collection.get(),
                native_selector_bson_ptr,
                impl::GetNative(options),
                write_result.GetNative(),
                error.GetNative()
            );
            break;
    }
    if (has_succeeded) {
        stopwatch.AccountSuccess();
    } else {
        stopwatch.AccountError(error.GetKind());
        if (operation.impl_->should_throw || !error.IsServerError()) {
            error.Throw("Error deleting documents");
        }
    }
    return write_result.Extract();
}

WriteResult CDriverCollectionImpl::Execute(const operations::FindAndModify& operation) {
    auto context = MakeRequestContext("mongo_find_and_modify", operation);

    auto options = CopyFindAndModifyOptions(operation.impl_->options);
    SetMaxServerTime(*options, operation.impl_->max_server_time, context);
    bool should_retry_dupkey = operation.impl_->should_retry_dupkey;

    while (true) {
        WriteResultHelper write_result;
        MongoError& error = write_result.GetError();
        stats::OperationStopwatch stopwatch(context.stats);
        const bson_t* native_fam_bson_ptr = operation.impl_->query.GetBson().get();
        if (mongoc_collection_find_and_modify_with_opts(
                context.collection.get(),
                native_fam_bson_ptr,
                options.get(),
                write_result.GetNative(),
                error.GetNative()
            ))
        {
            stopwatch.AccountSuccess();
        } else {
            auto error_kind = error.GetKind();
            stopwatch.AccountError(error_kind);
            if (should_retry_dupkey && error_kind == MongoError::Kind::kDuplicateKey) {
                should_retry_dupkey = false;
                continue;
            }
            error.Throw("Error running find and modify");
        }
        return write_result.Extract();
    }
}

WriteResult CDriverCollectionImpl::Execute(const operations::FindAndRemove& operation) {
    auto context = MakeRequestContext("mongo_find_and_delete", operation);

    auto options = CopyFindAndModifyOptions(operation.impl_->options);
    SetMaxServerTime(*options, operation.impl_->max_server_time, context);

    WriteResultHelper write_result;
    MongoError& error = write_result.GetError();
    stats::OperationStopwatch stopwatch(std::move(context.stats));
    const bson_t* native_fam_bson_ptr = operation.impl_->query.GetBson().get();
    if (mongoc_collection_find_and_modify_with_opts(
            context.collection.get(),
            native_fam_bson_ptr,
            options.get(),
            write_result.GetNative(),
            error.GetNative()
        ))
    {
        stopwatch.AccountSuccess();
    } else {
        stopwatch.AccountError(error.GetKind());
        error.Throw("Error running find and remove");
    }
    return write_result.Extract();
}

WriteResult CDriverCollectionImpl::Execute(operations::Bulk&& operation) {
    if (operation.IsEmpty()) {
        return {};
    }

    auto context = MakeRequestContext("mongo_bulk", operation);

    UASSERT(operation.impl_->bulk);
    mongoc_bulk_operation_set_database(operation.impl_->bulk.get(), GetDatabaseName().c_str());
    mongoc_bulk_operation_set_collection(operation.impl_->bulk.get(), GetCollectionName().c_str());

    mongoc_bulk_operation_set_client(operation.impl_->bulk.get(), context.client.get());

    WriteResultHelper write_result;
    MongoError& error = write_result.GetError();
    stats::OperationStopwatch stopwatch(std::move(context.stats));
    if (mongoc_bulk_operation_execute(operation.impl_->bulk.get(), write_result.GetNative(), error.GetNative())) {
        stopwatch.AccountSuccess();
    } else {
        stopwatch.AccountError(error.GetKind());
        if (operation.impl_->should_throw || !error.IsServerError()) {
            error.Throw("Error running bulk operation");
        }
    }
    return write_result.Extract();
}

Cursor CDriverCollectionImpl::Execute(const operations::Aggregate& operation) {
    auto context = MakeRequestContext("mongo_aggregate", operation);

    auto options = operation.impl_->options;
    SetMaxServerTime(options, operation.impl_->max_server_time, context);
    bool has_comment_option = operation.impl_->has_comment_option;
    if (!has_comment_option) {
        SetLinkComment(impl::EnsureBuilder(options), has_comment_option);
    }

    auto pipeline_doc = operation.impl_->pipeline.GetInternalArrayDocument();
    const bson_t* native_pipeline_bson_ptr = pipeline_doc.GetBson().get();
    impl::cdriver::CursorPtr cdriver_cursor(mongoc_collection_aggregate(
        context.collection.get(),
        MONGOC_QUERY_NONE,
        native_pipeline_bson_ptr,
        impl::GetNative(options),
        operation.impl_->read_prefs.Get()
    ));
    return Cursor(std::make_unique<impl::cdriver::CDriverCursorImpl>(
        std::move(context.client),
        std::move(cdriver_cursor),
        std::move(context.stats)
    ));
}

void CDriverCollectionImpl::Execute(const operations::Drop& operation) {
    auto context = MakeRequestContext("mongo_drop", operation);

    MongoError error;
    stats::OperationStopwatch stopwatch(std::move(context.stats));
    if (mongoc_collection_drop_with_opts(
            context.collection.get(),
            impl::GetNative(operation.impl_->options),
            error.GetNative()
        ))
    {
        stopwatch.AccountSuccess();
    } else {
        stopwatch.AccountError(error.GetKind());
        error.Throw("Error running drop");
    }
}

cdriver::CDriverPoolImpl& CDriverCollectionImpl::GetPool() const {
    // uasserted in ctor
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return *static_cast<cdriver::CDriverPoolImpl*>(pool_impl_.get());
}

cdriver::CDriverPoolImpl::BoundClientPtr CDriverCollectionImpl::GetClient(stats::OperationStatisticsItem& stats) const {
    try {
        return GetPool().Acquire();
    } catch (const CancelledException& ex) {
        stats.Account(stats::ErrorType::kCancelled);
        auto& span = tracing::Span::CurrentSpan();
        if (ex.IsByDeadlinePropagation()) {
            span.AddTag(kCancelledByDeadlineTag, true);
        } else {
            span.AddTag(kCancelledTag, true);
        }
        throw;
    } catch (const PoolOverloadException& /*ex*/) {
        stats.Account(stats::ErrorType::kPoolOverload);
        throw;
    }
}

mongoc_client_session_t* CDriverCollectionImpl::GetSession() const { return nullptr; }

RequestContext CDriverCollectionImpl::MakeRequestContext(std::string&& span_name, const stats::OperationKey& stats_key)
    const {
    auto span = MakeSpan(std::move(span_name));
    auto stats = statistics_->items[stats_key];
    auto dynamic_config = pool_impl_->GetConfig();

    // first deadline check, to make sure we dont get/wait for client if deadline is already reached.
    auto timeout_ms = GetTimeoutOrThrow(dynamic_config, *stats, span);
    if (timeout_ms) {
        span.AddTag(tracing::kTimeoutMs, timeout_ms->count());
    }

    auto client = GetClient(*stats);
    cdriver::CollectionPtr
        collection(mongoc_client_get_collection(client.get(), GetDatabaseName().c_str(), GetCollectionName().c_str()));

    // The second deadline check, to make sure we did not hit deadline after waiting or creating a new client.
    timeout_ms = timeout_ms ? GetTimeoutOrThrow(dynamic_config, *stats, span) : timeout_ms;
    if (timeout_ms) {
        span.AddTag(tracing::kTimeoutMs, timeout_ms->count());
    }

    return RequestContext{
        std::move(stats),
        std::move(dynamic_config),
        std::move(client),
        std::move(collection),
        std::move(span),
        timeout_ms,
    };
}

template <typename Operation>
RequestContext CDriverCollectionImpl::MakeRequestContext(std::string&& span_name, const Operation& operation) const {
    return MakeRequestContext(std::move(span_name), operation.impl_->op_key);
}

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
