#include <storages/mongo/cdriver/collection_impl.hpp>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/mongo_error.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
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

size_t CDriverCollectionImpl::Execute(const operations::Count& count_op) const {
  auto span = MakeSpan("mongo_count");
  auto op_stats = statistics_->items[count_op.impl_->op_key];
  auto [client, collection] = GetCDriverCollection();

  auto options = count_op.impl_->options;
  SetMaxServerTime(options, count_op.impl_->max_server_time);

  MongoError error;
  stats::OperationStopwatch count_sw(std::move(op_stats));
  const bson_t* native_filter_bson_ptr = count_op.impl_->filter.GetBson().get();
  int64_t count = -1;
  if (count_op.impl_->use_new_count) {
    count = mongoc_collection_count_documents(
        collection.get(), native_filter_bson_ptr,
        impl::GetNative(count_op.impl_->options),
        count_op.impl_->read_prefs.Get(), nullptr, error.GetNative());
  } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"  // i know
    count = mongoc_collection_count_with_opts(
        collection.get(), MONGOC_QUERY_NONE, native_filter_bson_ptr,  //
        0, 0,  // skip and limit are set in options
        impl::GetNative(count_op.impl_->options),
        count_op.impl_->read_prefs.Get(), error.GetNative());
#pragma clang diagnostic pop
  }
  if (count < 0) {
    count_sw.AccountError(error.GetKind());
    error.Throw("Error counting documents");
  }
  count_sw.AccountSuccess();
  return count;
}

size_t CDriverCollectionImpl::Execute(
    const operations::CountApprox& count_approx_op) const {
  auto span = MakeSpan("mongo_count_approx");
  auto op_stats = statistics_->items[count_approx_op.impl_->op_key];
  auto [client, collection] = GetCDriverCollection();

  auto options = count_approx_op.impl_->options;
  SetMaxServerTime(options, count_approx_op.impl_->max_server_time);

  MongoError error;
  stats::OperationStopwatch count_approx_sw(std::move(op_stats));
  auto count = mongoc_collection_estimated_document_count(
      collection.get(), impl::GetNative(count_approx_op.impl_->options),
      count_approx_op.impl_->read_prefs.Get(), nullptr, error.GetNative());
  if (count < 0) {
    count_approx_sw.AccountError(error.GetKind());
    error.Throw("Error counting documents");
  }
  count_approx_sw.AccountSuccess();
  return count;
}

Cursor CDriverCollectionImpl::Execute(const operations::Find& find_op) const {
  auto span = MakeSpan("mongo_find");
  auto op_stats = statistics_->items[find_op.impl_->op_key];
  auto [client, collection] = GetCDriverCollection();

  auto options = find_op.impl_->options;
  SetMaxServerTime(options, find_op.impl_->max_server_time);
  bool has_comment_option = find_op.impl_->has_comment_option;
  if (!has_comment_option)
    SetLinkComment(impl::EnsureBuilder(options), has_comment_option);

  const bson_t* native_filter_bson_ptr = find_op.impl_->filter.GetBson().get();
  impl::cdriver::CursorPtr cdriver_cursor(mongoc_collection_find_with_opts(
      collection.get(), native_filter_bson_ptr, impl::GetNative(options),
      find_op.impl_->read_prefs.Get()));
  return Cursor(std::make_unique<impl::cdriver::CDriverCursorImpl>(
      std::move(client), std::move(cdriver_cursor), std::move(op_stats)));
}

WriteResult CDriverCollectionImpl::Execute(
    const operations::InsertOne& insert_op) {
  auto span = MakeSpan("mongo_insert_one");
  auto op_stats = statistics_->items[insert_op.impl_->op_key];
  auto [client, collection] = GetCDriverCollection();

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch insert_sw(std::move(op_stats));
  const bson_t* native_bson_ptr = insert_op.impl_->document.GetBson().get();
  if (mongoc_collection_insert_one(collection.get(), native_bson_ptr,
                                   impl::GetNative(insert_op.impl_->options),
                                   write_result.GetNative(),
                                   error.GetNative())) {
    insert_sw.AccountSuccess();
  } else {
    insert_sw.AccountError(error.GetKind());
    if (insert_op.impl_->should_throw || !error.IsServerError()) {
      error.Throw("Error inserting document");
    }
  }
  return write_result.Extract();
}

WriteResult CDriverCollectionImpl::Execute(
    const operations::InsertMany& insert_op) {
  auto span = MakeSpan("mongo_insert_many");
  if (insert_op.impl_->documents.empty()) return {};

  std::vector<const bson_t*> bsons;
  bsons.reserve(insert_op.impl_->documents.size());
  for (const auto& doc : insert_op.impl_->documents) {
    bsons.push_back(doc.GetBson().get());
  }

  auto op_stats = statistics_->items[insert_op.impl_->op_key];
  auto [client, collection] = GetCDriverCollection();

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch insert_sw(std::move(op_stats));
  if (mongoc_collection_insert_many(
          collection.get(), bsons.data(), bsons.size(),
          impl::GetNative(insert_op.impl_->options), write_result.GetNative(),
          error.GetNative())) {
    insert_sw.AccountSuccess();
  } else {
    insert_sw.AccountError(error.GetKind());
    if (insert_op.impl_->should_throw || !error.IsServerError()) {
      error.Throw("Error inserting documents");
    }
  }
  return write_result.Extract();
}

WriteResult CDriverCollectionImpl::Execute(
    const operations::ReplaceOne& replace_op) {
  auto span = MakeSpan("mongo_replace_one");
  auto op_stats = statistics_->items[replace_op.impl_->op_key];
  auto [client, collection] = GetCDriverCollection();

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch replace_sw(std::move(op_stats));
  const bson_t* native_selector_bson_ptr =
      replace_op.impl_->selector.GetBson().get();
  const bson_t* native_replacement_bson_ptr =
      replace_op.impl_->replacement.GetBson().get();
  if (mongoc_collection_replace_one(collection.get(), native_selector_bson_ptr,
                                    native_replacement_bson_ptr,
                                    impl::GetNative(replace_op.impl_->options),
                                    write_result.GetNative(),
                                    error.GetNative())) {
    replace_sw.AccountSuccess();
  } else {
    replace_sw.AccountError(error.GetKind());
    if (replace_op.impl_->should_throw || !error.IsServerError()) {
      error.Throw("Error replacing document");
    }
  }
  return write_result.Extract();
}

WriteResult CDriverCollectionImpl::Execute(
    const operations::Update& update_op) {
  auto span = MakeSpan("mongo_update");
  const auto op_stats = statistics_->items[update_op.impl_->op_key];
  auto [client, collection] = GetCDriverCollection();

  bool should_retry_dupkey = update_op.impl_->should_retry_dupkey;
  while (true) {
    MongoError error;
    WriteResultHelper write_result;
    stats::OperationStopwatch update_sw(op_stats);
    const bson_t* native_selector_bson_ptr =
        update_op.impl_->selector.GetBson().get();
    const bson_t* native_update_bson_ptr =
        update_op.impl_->update.GetBson().get();
    bool has_succeeded = false;
    switch (update_op.impl_->mode) {
      case operations::Update::Mode::kSingle:
        has_succeeded = mongoc_collection_update_one(
            collection.get(), native_selector_bson_ptr, native_update_bson_ptr,
            impl::GetNative(update_op.impl_->options), write_result.GetNative(),
            error.GetNative());
        break;

      case operations::Update::Mode::kMulti:
        has_succeeded = mongoc_collection_update_many(
            collection.get(), native_selector_bson_ptr, native_update_bson_ptr,
            impl::GetNative(update_op.impl_->options), write_result.GetNative(),
            error.GetNative());
        break;
    }
    if (has_succeeded) {
      update_sw.AccountSuccess();
    } else {
      auto error_kind = error.GetKind();
      update_sw.AccountError(error_kind);
      if (should_retry_dupkey &&
          error_kind == MongoError::Kind::kDuplicateKey) {
        UASSERT(update_op.impl_->mode == operations::Update::Mode::kSingle);
        should_retry_dupkey = false;
        continue;
      }
      if (update_op.impl_->should_throw || !error.IsServerError()) {
        error.Throw("Error updating documents");
      }
    }
    return write_result.Extract();
  }
}

WriteResult CDriverCollectionImpl::Execute(
    const operations::Delete& delete_op) {
  auto span = MakeSpan("mongo_delete");
  auto op_stats = statistics_->items[delete_op.impl_->op_key];
  auto [client, collection] = GetCDriverCollection();

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch delete_sw(std::move(op_stats));
  const bson_t* native_selector_bson_ptr =
      delete_op.impl_->selector.GetBson().get();
  bool has_succeeded = false;
  switch (delete_op.impl_->mode) {
    case operations::Delete::Mode::kSingle:
      has_succeeded = mongoc_collection_delete_one(
          collection.get(), native_selector_bson_ptr,
          impl::GetNative(delete_op.impl_->options), write_result.GetNative(),
          error.GetNative());
      break;

    case operations::Delete::Mode::kMulti:
      has_succeeded = mongoc_collection_delete_many(
          collection.get(), native_selector_bson_ptr,
          impl::GetNative(delete_op.impl_->options), write_result.GetNative(),
          error.GetNative());
      break;
  }
  if (has_succeeded) {
    delete_sw.AccountSuccess();
  } else {
    delete_sw.AccountError(error.GetKind());
    if (delete_op.impl_->should_throw || !error.IsServerError()) {
      error.Throw("Error deleting documents");
    }
  }
  return write_result.Extract();
}

WriteResult CDriverCollectionImpl::Execute(
    const operations::FindAndModify& fam_op) {
  auto span = MakeSpan("mongo_find_and_modify");
  const auto op_stats = statistics_->items[fam_op.impl_->op_key];
  auto [client, collection] = GetCDriverCollection();

  auto options = CopyFindAndModifyOptions(fam_op.impl_->options);
  SetMaxServerTime(*options, fam_op.impl_->max_server_time);
  bool should_retry_dupkey = fam_op.impl_->should_retry_dupkey;

  while (true) {
    MongoError error;
    WriteResultHelper write_result;
    stats::OperationStopwatch fam_sw(op_stats);
    const bson_t* native_fam_bson_ptr = fam_op.impl_->query.GetBson().get();
    if (mongoc_collection_find_and_modify_with_opts(
            collection.get(), native_fam_bson_ptr, options.get(),
            write_result.GetNative(), error.GetNative())) {
      fam_sw.AccountSuccess();
    } else {
      auto error_kind = error.GetKind();
      fam_sw.AccountError(error_kind);
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
    const operations::FindAndRemove& fam_op) {
  auto span = MakeSpan("mongo_find_and_delete");
  auto op_stats = statistics_->items[fam_op.impl_->op_key];
  auto [client, collection] = GetCDriverCollection();

  auto options = CopyFindAndModifyOptions(fam_op.impl_->options);
  SetMaxServerTime(*options, fam_op.impl_->max_server_time);

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch fam_sw(std::move(op_stats));
  const bson_t* native_fam_bson_ptr = fam_op.impl_->query.GetBson().get();
  if (mongoc_collection_find_and_modify_with_opts(
          collection.get(), native_fam_bson_ptr, options.get(),
          write_result.GetNative(), error.GetNative())) {
    fam_sw.AccountSuccess();
  } else {
    fam_sw.AccountError(error.GetKind());
    error.Throw("Error running find and remove");
  }
  return write_result.Extract();
}

WriteResult CDriverCollectionImpl::Execute(operations::Bulk&& bulk_op) {
  auto span = MakeSpan("mongo_bulk");
  if (bulk_op.IsEmpty()) return {};

  UASSERT(bulk_op.impl_->bulk);
  mongoc_bulk_operation_set_database(bulk_op.impl_->bulk.get(),
                                     GetDatabaseName().c_str());
  mongoc_bulk_operation_set_collection(bulk_op.impl_->bulk.get(),
                                       GetCollectionName().c_str());

  auto op_stats = statistics_->items[bulk_op.impl_->op_key];
  auto client = GetCDriverClient();
  mongoc_bulk_operation_set_client(bulk_op.impl_->bulk.get(), client.get());

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch bulk_sw(std::move(op_stats));
  if (mongoc_bulk_operation_execute(bulk_op.impl_->bulk.get(),
                                    write_result.GetNative(),
                                    error.GetNative())) {
    bulk_sw.AccountSuccess();
  } else {
    bulk_sw.AccountError(error.GetKind());
    if (bulk_op.impl_->should_throw || !error.IsServerError()) {
      error.Throw("Error running bulk operation");
    }
  }
  return write_result.Extract();
}

Cursor CDriverCollectionImpl::Execute(
    const operations::Aggregate& aggregate_op) {
  auto span = MakeSpan("mongo_aggregate");
  auto op_stats = statistics_->items[aggregate_op.impl_->op_key];
  auto [client, collection] = GetCDriverCollection();

  auto options = aggregate_op.impl_->options;
  SetMaxServerTime(options, aggregate_op.impl_->max_server_time);
  bool has_comment_option = aggregate_op.impl_->has_comment_option;
  if (!has_comment_option)
    SetLinkComment(impl::EnsureBuilder(options), has_comment_option);

  auto pipeline_doc = aggregate_op.impl_->pipeline.GetInternalArrayDocument();
  const bson_t* native_pipeline_bson_ptr = pipeline_doc.GetBson().get();
  impl::cdriver::CursorPtr cdriver_cursor(mongoc_collection_aggregate(
      collection.get(), MONGOC_QUERY_NONE, native_pipeline_bson_ptr,
      impl::GetNative(options), aggregate_op.impl_->read_prefs.Get()));
  return Cursor(std::make_unique<impl::cdriver::CDriverCursorImpl>(
      std::move(client), std::move(cdriver_cursor), std::move(op_stats)));
}

void CDriverCollectionImpl::Execute(const operations::Drop& drop_op) {
  auto span = MakeSpan("mongo_drop");
  auto op_stats = statistics_->items[drop_op.impl_->op_key];
  auto [client, collection] = GetCDriverCollection();

  MongoError error;
  stats::OperationStopwatch drop_sw(std::move(op_stats));
  if (mongoc_collection_drop_with_opts(collection.get(),
                                       impl::GetNative(drop_op.impl_->options),
                                       error.GetNative())) {
    drop_sw.AccountSuccess();
  } else {
    drop_sw.AccountError(error.GetKind());
    error.Throw("Error running drop");
  }
}

cdriver::CDriverPoolImpl::BoundClientPtr
CDriverCollectionImpl::GetCDriverClient() const {
  // uasserted in ctor
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  return static_cast<cdriver::CDriverPoolImpl*>(pool_impl_.get())->Acquire();
}

std::tuple<cdriver::CDriverPoolImpl::BoundClientPtr, cdriver::CollectionPtr>
CDriverCollectionImpl::GetCDriverCollection() const {
  auto client = GetCDriverClient();
  cdriver::CollectionPtr collection(mongoc_client_get_collection(
      client.get(), GetDatabaseName().c_str(), GetCollectionName().c_str()));
  return std::make_tuple(std::move(client), std::move(collection));
}

std::chrono::milliseconds CDriverCollectionImpl::ComputeAdjustedMaxServerTime(
    std::chrono::milliseconds user_max_server_time) const {
  auto max_server_time = user_max_server_time;
  operations::VerifyMaxServerTime(max_server_time);

  if (max_server_time == operations::kNoMaxServerTime) {
    const auto dynamic_config = pool_impl_->GetConfig();
    max_server_time = dynamic_config[kDefaultMaxTime];
  }

  return max_server_time;
}

void CDriverCollectionImpl::SetMaxServerTime(
    std::optional<formats::bson::impl::BsonBuilder>& builder,
    std::chrono::milliseconds max_server_time) const {
  max_server_time = ComputeAdjustedMaxServerTime(max_server_time);
  if (max_server_time == operations::kNoMaxServerTime) return;

  constexpr std::string_view kOptionName = "maxTimeMS";
  impl::EnsureBuilder(builder).Append(kOptionName, max_server_time.count());
}

void CDriverCollectionImpl::SetMaxServerTime(
    mongoc_find_and_modify_opts_t& options,
    std::chrono::milliseconds max_server_time) const {
  max_server_time = ComputeAdjustedMaxServerTime(max_server_time);
  if (max_server_time == operations::kNoMaxServerTime) return;

  if (!mongoc_find_and_modify_opts_set_max_time_ms(&options,
                                                   max_server_time.count())) {
    throw MongoException("Cannot set max server time");
  }
}

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
