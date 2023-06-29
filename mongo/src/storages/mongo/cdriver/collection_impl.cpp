#include <storages/mongo/cdriver/collection_impl.hpp>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/mongo_error.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/utils/text.hpp>

#include <formats/bson/wrappers.hpp>
#include <storages/mongo/cdriver/cursor_impl.hpp>
#include <storages/mongo/cdriver/pool_impl.hpp>
#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/operations_common.hpp>
#include <storages/mongo/operations_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {
namespace {

const std::string kCancelledByDeadlineTag = "cancelled_by_deadline";
const std::string kCancelledTag = "cancelled";
const std::string kMaxTimeMsTag = "max_time_ms";

class WriteResultHelper {
 public:
  bson_t* GetNative() { return bson_.Get(); }

  WriteResult Extract() {
    return WriteResult(formats::bson::Document(bson_.Extract()));
  }

 private:
  formats::bson::impl::UninitializedBson bson_;
};

std::optional<std::string> GetCurrentSpanLink() {
  auto* span = tracing::Span::CurrentSpanUnchecked();
  if (span) return span->GetLink();
  return std::nullopt;
}

void SetLinkComment(formats::bson::impl::BsonBuilder& builder,
                    bool& has_comment_option) {
  auto link = GetCurrentSpanLink();
  if (link)
    operations::AppendComment(builder, has_comment_option,
                              options::Comment("link=" + *link));
}

impl::cdriver::FindAndModifyOptsPtr CopyFindAndModifyOptions(
    const impl::cdriver::FindAndModifyOptsPtr& options) {
  impl::cdriver::FindAndModifyOptsPtr result(mongoc_find_and_modify_opts_new());

  {
    formats::bson::impl::UninitializedBson tmp_bson;
    mongoc_find_and_modify_opts_get_sort(options.get(), tmp_bson.Get());
    if (!mongoc_find_and_modify_opts_set_sort(result.get(), tmp_bson.Get()))
      throw MongoException("Cannot set 'sort'");
  }

  auto flags = mongoc_find_and_modify_opts_get_flags(options.get());
  if (!mongoc_find_and_modify_opts_set_flags(result.get(), flags))
    throw MongoException("Cannot set 'flags'");

  if (!(flags & MONGOC_FIND_AND_MODIFY_REMOVE)) {
    formats::bson::impl::UninitializedBson tmp_bson;
    mongoc_find_and_modify_opts_get_update(options.get(), tmp_bson.Get());
    if (!mongoc_find_and_modify_opts_set_update(result.get(), tmp_bson.Get()))
      throw MongoException("Cannot set 'update'");
  }

  {
    formats::bson::impl::UninitializedBson tmp_bson;
    mongoc_find_and_modify_opts_get_fields(options.get(), tmp_bson.Get());
    if (!mongoc_find_and_modify_opts_set_fields(result.get(), tmp_bson.Get()))
      throw MongoException("Cannot set 'fields'");
  }

  {
    auto bypass_document_validation =
        mongoc_find_and_modify_opts_get_bypass_document_validation(
            options.get());
    if (!mongoc_find_and_modify_opts_set_bypass_document_validation(
            result.get(), bypass_document_validation))
      throw MongoException("Cannot set 'bypass_document_validation'");
  }

  {
    auto max_time_ms =
        mongoc_find_and_modify_opts_get_max_time_ms(options.get());
    if (!mongoc_find_and_modify_opts_set_max_time_ms(result.get(), max_time_ms))
      throw MongoException("Cannot set 'max_time_ms'");
  }

  {
    formats::bson::impl::UninitializedBson tmp_bson;
    mongoc_find_and_modify_opts_get_extra(options.get(), tmp_bson.Get());
    if (!mongoc_find_and_modify_opts_append(result.get(), tmp_bson.Get()))
      throw MongoException("Cannot set 'extra'");
  }

  return result;
}

std::optional<std::chrono::milliseconds> GetDeadlineTimeLeft(
    const dynamic_config::Snapshot& config) {
  if (!config[kDeadlinePropagationEnabled]) return std::nullopt;

  const auto inherited_deadline = server::request::GetTaskInheritedDeadline();
  if (!inherited_deadline.IsReachable()) return std::nullopt;

  const auto inherited_timeout =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          inherited_deadline.TimeLeftApprox());
  return inherited_timeout;
}

std::chrono::milliseconds ComputeAdjustedMaxServerTime(
    std::chrono::milliseconds user_max_server_time,
    const RequestContext& context) {
  auto max_server_time = user_max_server_time;
  try {
    operations::VerifyMaxServerTime(max_server_time);
  } catch (const InvalidQueryArgumentException& /*ex*/) {
    context.stats->Account(stats::ErrorType::kBadQueryArgument);
    throw;
  }

  if (max_server_time == operations::kNoMaxServerTime) {
    max_server_time = context.dynamic_config[kDefaultMaxTime];
  }

  if (auto inherited_deadline = context.inherited_deadline) {
    operations::VerifyMaxServerTime(*inherited_deadline);
    if (max_server_time == operations::kNoMaxServerTime ||
        *inherited_deadline < max_server_time) {
      max_server_time = *inherited_deadline;
    }
  }

  if (max_server_time != operations::kNoMaxServerTime) {
    tracing::Span::CurrentSpan().AddTag(kMaxTimeMsTag, max_server_time.count());
  }

  return max_server_time;
}

void SetMaxServerTime(std::optional<formats::bson::impl::BsonBuilder>& builder,
                      std::chrono::milliseconds max_server_time,
                      const RequestContext& context) {
  max_server_time = ComputeAdjustedMaxServerTime(max_server_time, context);
  if (max_server_time == operations::kNoMaxServerTime) return;

  constexpr std::string_view kOptionName = "maxTimeMS";
  impl::EnsureBuilder(builder).Append(kOptionName, max_server_time.count());
}

void SetMaxServerTime(mongoc_find_and_modify_opts_t& options,
                      std::chrono::milliseconds max_server_time,
                      const RequestContext& context) {
  max_server_time = ComputeAdjustedMaxServerTime(max_server_time, context);
  if (max_server_time == operations::kNoMaxServerTime) return;

  if (!mongoc_find_and_modify_opts_set_max_time_ms(&options,
                                                   max_server_time.count())) {
    throw MongoException("Cannot set max server time");
  }
}

}  // namespace

CDriverCollectionImpl::CDriverCollectionImpl(PoolImplPtr pool_impl,
                                             std::string database_name,
                                             std::string collection_name)
    : CollectionImpl(std::move(database_name), std::move(collection_name)),
      pool_impl_(std::move(pool_impl)),
      statistics_(
          pool_impl_->GetStatistics().collections[GetCollectionName()]) {
  UASSERT(dynamic_cast<cdriver::CDriverPoolImpl*>(pool_impl_.get()));
}

size_t CDriverCollectionImpl::Execute(
    const operations::Count& operation) const {
  auto context = MakeRequestContext("mongo_count", operation);

  auto options = operation.impl_->options;
  SetMaxServerTime(options, operation.impl_->max_server_time, context);

  MongoError error;
  stats::OperationStopwatch stopwatch(std::move(context.stats));
  const bson_t* native_filter_bson_ptr =
      operation.impl_->filter.GetBson().get();
  int64_t count = -1;
  if (operation.impl_->use_new_count) {
    count = mongoc_collection_count_documents(
        context.collection.get(), native_filter_bson_ptr,
        impl::GetNative(operation.impl_->options),
        operation.impl_->read_prefs.Get(), nullptr, error.GetNative());
  } else {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"  // i know
#endif
    count = mongoc_collection_count_with_opts(
        context.collection.get(), MONGOC_QUERY_NONE, native_filter_bson_ptr,  //
        0, 0,  // skip and limit are set in options
        impl::GetNative(operation.impl_->options),
        operation.impl_->read_prefs.Get(), error.GetNative());
#ifdef __clang__
#pragma clang diagnostic pop
#endif
  }
  if (count < 0) {
    stopwatch.AccountError(error.GetKind());
    error.Throw("Error counting documents");
  }
  stopwatch.AccountSuccess();
  return count;
}

size_t CDriverCollectionImpl::Execute(
    const operations::CountApprox& operation) const {
  auto context = MakeRequestContext("mongo_count_approx", operation);

  auto options = operation.impl_->options;
  SetMaxServerTime(options, operation.impl_->max_server_time, context);

  MongoError error;
  stats::OperationStopwatch stopwatch(std::move(context.stats));
  auto count = mongoc_collection_estimated_document_count(
      context.collection.get(), impl::GetNative(operation.impl_->options),
      operation.impl_->read_prefs.Get(), nullptr, error.GetNative());
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
  if (!has_comment_option)
    SetLinkComment(impl::EnsureBuilder(options), has_comment_option);

  const bson_t* native_filter_bson_ptr =
      operation.impl_->filter.GetBson().get();
  impl::cdriver::CursorPtr cdriver_cursor(mongoc_collection_find_with_opts(
      context.collection.get(), native_filter_bson_ptr,
      impl::GetNative(options), operation.impl_->read_prefs.Get()));
  return Cursor(std::make_unique<impl::cdriver::CDriverCursorImpl>(
      std::move(context.client), std::move(cdriver_cursor),
      std::move(context.stats)));
}

WriteResult CDriverCollectionImpl::Execute(
    const operations::InsertOne& operation) {
  auto context = MakeRequestContext("mongo_insert_one", operation);

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch stopwatch(std::move(context.stats));
  const bson_t* native_bson_ptr = operation.impl_->document.GetBson().get();
  if (mongoc_collection_insert_one(context.collection.get(), native_bson_ptr,
                                   impl::GetNative(operation.impl_->options),
                                   write_result.GetNative(),
                                   error.GetNative())) {
    stopwatch.AccountSuccess();
  } else {
    stopwatch.AccountError(error.GetKind());
    if (operation.impl_->should_throw || !error.IsServerError()) {
      error.Throw("Error inserting document");
    }
  }
  return write_result.Extract();
}

WriteResult CDriverCollectionImpl::Execute(
    const operations::InsertMany& operation) {
  if (operation.impl_->documents.empty()) return {};

  auto context = MakeRequestContext("mongo_insert_many", operation);

  std::vector<const bson_t*> bsons;
  bsons.reserve(operation.impl_->documents.size());
  for (const auto& doc : operation.impl_->documents) {
    bsons.push_back(doc.GetBson().get());
  }

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch stopwatch(std::move(context.stats));
  if (mongoc_collection_insert_many(
          context.collection.get(), bsons.data(), bsons.size(),
          impl::GetNative(operation.impl_->options), write_result.GetNative(),
          error.GetNative())) {
    stopwatch.AccountSuccess();
  } else {
    stopwatch.AccountError(error.GetKind());
    if (operation.impl_->should_throw || !error.IsServerError()) {
      error.Throw("Error inserting documents");
    }
  }
  return write_result.Extract();
}

WriteResult CDriverCollectionImpl::Execute(
    const operations::ReplaceOne& operation) {
  auto context = MakeRequestContext("mongo_replace_one", operation);

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch stopwatch(std::move(context.stats));
  const bson_t* native_selector_bson_ptr =
      operation.impl_->selector.GetBson().get();
  const bson_t* native_replacement_bson_ptr =
      operation.impl_->replacement.GetBson().get();
  if (mongoc_collection_replace_one(
          context.collection.get(), native_selector_bson_ptr,
          native_replacement_bson_ptr,
          impl::GetNative(operation.impl_->options), write_result.GetNative(),
          error.GetNative())) {
    stopwatch.AccountSuccess();
  } else {
    stopwatch.AccountError(error.GetKind());
    if (operation.impl_->should_throw || !error.IsServerError()) {
      error.Throw("Error replacing document");
    }
  }
  return write_result.Extract();
}

WriteResult CDriverCollectionImpl::Execute(
    const operations::Update& operation) {
  auto context = MakeRequestContext("mongo_update", operation);

  bool should_retry_dupkey = operation.impl_->should_retry_dupkey;
  while (true) {
    MongoError error;
    WriteResultHelper write_result;
    stats::OperationStopwatch stopwatch(context.stats);
    const bson_t* native_selector_bson_ptr =
        operation.impl_->selector.GetBson().get();
    const bson_t* native_update_bson_ptr =
        operation.impl_->update.GetBson().get();
    bool has_succeeded = false;
    switch (operation.impl_->mode) {
      case operations::Update::Mode::kSingle:
        has_succeeded = mongoc_collection_update_one(
            context.collection.get(), native_selector_bson_ptr,
            native_update_bson_ptr, impl::GetNative(operation.impl_->options),
            write_result.GetNative(), error.GetNative());
        break;

      case operations::Update::Mode::kMulti:
        has_succeeded = mongoc_collection_update_many(
            context.collection.get(), native_selector_bson_ptr,
            native_update_bson_ptr, impl::GetNative(operation.impl_->options),
            write_result.GetNative(), error.GetNative());
        break;
    }
    if (has_succeeded) {
      stopwatch.AccountSuccess();
    } else {
      auto error_kind = error.GetKind();
      stopwatch.AccountError(error_kind);
      if (should_retry_dupkey &&
          error_kind == MongoError::Kind::kDuplicateKey) {
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

WriteResult CDriverCollectionImpl::Execute(
    const operations::Delete& operation) {
  auto context = MakeRequestContext("mongo_delete", operation);

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch stopwatch(std::move(context.stats));
  const bson_t* native_selector_bson_ptr =
      operation.impl_->selector.GetBson().get();
  bool has_succeeded = false;
  switch (operation.impl_->mode) {
    case operations::Delete::Mode::kSingle:
      has_succeeded = mongoc_collection_delete_one(
          context.collection.get(), native_selector_bson_ptr,
          impl::GetNative(operation.impl_->options), write_result.GetNative(),
          error.GetNative());
      break;

    case operations::Delete::Mode::kMulti:
      has_succeeded = mongoc_collection_delete_many(
          context.collection.get(), native_selector_bson_ptr,
          impl::GetNative(operation.impl_->options), write_result.GetNative(),
          error.GetNative());
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

WriteResult CDriverCollectionImpl::Execute(
    const operations::FindAndModify& operation) {
  auto context = MakeRequestContext("mongo_find_and_modify", operation);

  auto options = CopyFindAndModifyOptions(operation.impl_->options);
  SetMaxServerTime(*options, operation.impl_->max_server_time, context);
  bool should_retry_dupkey = operation.impl_->should_retry_dupkey;

  while (true) {
    MongoError error;
    WriteResultHelper write_result;
    stats::OperationStopwatch stopwatch(context.stats);
    const bson_t* native_fam_bson_ptr = operation.impl_->query.GetBson().get();
    if (mongoc_collection_find_and_modify_with_opts(
            context.collection.get(), native_fam_bson_ptr, options.get(),
            write_result.GetNative(), error.GetNative())) {
      stopwatch.AccountSuccess();
    } else {
      auto error_kind = error.GetKind();
      stopwatch.AccountError(error_kind);
      if (should_retry_dupkey &&
          error_kind == MongoError::Kind::kDuplicateKey) {
        should_retry_dupkey = false;
        continue;
      }
      error.Throw("Error running find and modify");
    }
    return write_result.Extract();
  }
}

WriteResult CDriverCollectionImpl::Execute(
    const operations::FindAndRemove& operation) {
  auto context = MakeRequestContext("mongo_find_and_delete", operation);

  auto options = CopyFindAndModifyOptions(operation.impl_->options);
  SetMaxServerTime(*options, operation.impl_->max_server_time, context);

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch stopwatch(std::move(context.stats));
  const bson_t* native_fam_bson_ptr = operation.impl_->query.GetBson().get();
  if (mongoc_collection_find_and_modify_with_opts(
          context.collection.get(), native_fam_bson_ptr, options.get(),
          write_result.GetNative(), error.GetNative())) {
    stopwatch.AccountSuccess();
  } else {
    stopwatch.AccountError(error.GetKind());
    error.Throw("Error running find and remove");
  }
  return write_result.Extract();
}

WriteResult CDriverCollectionImpl::Execute(operations::Bulk&& operation) {
  if (operation.IsEmpty()) return {};

  auto context = MakeRequestContext("mongo_bulk", operation);

  UASSERT(operation.impl_->bulk);
  mongoc_bulk_operation_set_database(operation.impl_->bulk.get(),
                                     GetDatabaseName().c_str());
  mongoc_bulk_operation_set_collection(operation.impl_->bulk.get(),
                                       GetCollectionName().c_str());

  mongoc_bulk_operation_set_client(operation.impl_->bulk.get(),
                                   context.client.get());

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch stopwatch(std::move(context.stats));
  if (mongoc_bulk_operation_execute(operation.impl_->bulk.get(),
                                    write_result.GetNative(),
                                    error.GetNative())) {
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
  if (!has_comment_option)
    SetLinkComment(impl::EnsureBuilder(options), has_comment_option);

  auto pipeline_doc = operation.impl_->pipeline.GetInternalArrayDocument();
  const bson_t* native_pipeline_bson_ptr = pipeline_doc.GetBson().get();
  impl::cdriver::CursorPtr cdriver_cursor(mongoc_collection_aggregate(
      context.collection.get(), MONGOC_QUERY_NONE, native_pipeline_bson_ptr,
      impl::GetNative(options), operation.impl_->read_prefs.Get()));
  return Cursor(std::make_unique<impl::cdriver::CDriverCursorImpl>(
      std::move(context.client), std::move(cdriver_cursor),
      std::move(context.stats)));
}

void CDriverCollectionImpl::Execute(const operations::Drop& operation) {
  auto context = MakeRequestContext("mongo_drop", operation);

  MongoError error;
  stats::OperationStopwatch stopwatch(std::move(context.stats));
  if (mongoc_collection_drop_with_opts(
          context.collection.get(), impl::GetNative(operation.impl_->options),
          error.GetNative())) {
    stopwatch.AccountSuccess();
  } else {
    stopwatch.AccountError(error.GetKind());
    error.Throw("Error running drop");
  }
}

cdriver::CDriverPoolImpl::BoundClientPtr CDriverCollectionImpl::GetClient(
    stats::OperationStatisticsItem& stats) const {
  try {
    // uasserted in ctor
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return static_cast<cdriver::CDriverPoolImpl*>(pool_impl_.get())->Acquire();
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

RequestContext CDriverCollectionImpl::MakeRequestContext(
    std::string&& span_name, const stats::OperationKey& stats_key) const {
  auto span = MakeSpan(std::move(span_name));
  auto stats = statistics_->items[stats_key];
  auto dynamic_config = pool_impl_->GetConfig();

  const auto inherited_deadline = GetDeadlineTimeLeft(dynamic_config);
  if (inherited_deadline && inherited_deadline <= std::chrono::seconds{0}) {
    stats->Account(stats::ErrorType::kCancelled);
    span.AddTag(kCancelledByDeadlineTag, true);
    throw CancelledException(CancelledException::ByDeadlinePropagation{});
  }

  if (inherited_deadline) {
    span.AddTag(tracing::kTimeoutMs, inherited_deadline->count());
  }

  auto client = GetClient(*stats);
  cdriver::CollectionPtr collection(mongoc_client_get_collection(
      client.get(), GetDatabaseName().c_str(), GetCollectionName().c_str()));

  return RequestContext{
      std::move(stats),      std::move(dynamic_config), std::move(client),
      std::move(collection), std::move(span),           inherited_deadline,
  };
}

template <typename Operation>
RequestContext CDriverCollectionImpl::MakeRequestContext(
    std::string&& span_name, const Operation& operation) const {
  return MakeRequestContext(std::move(span_name), operation.impl_->op_key);
}

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
