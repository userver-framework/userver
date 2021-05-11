#include <storages/mongo/cdriver/collection_impl.hpp>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <formats/bson/document.hpp>
#include <formats/bson/inline.hpp>
#include <storages/mongo/exception.hpp>
#include <storages/mongo/mongo_error.hpp>
#include <utils/assert.hpp>
#include <utils/text.hpp>

#include <formats/bson/wrappers.hpp>
#include <storages/mongo/cdriver/cursor_impl.hpp>
#include <storages/mongo/cdriver/pool_impl.hpp>
#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/operations_common.hpp>
#include <storages/mongo/operations_impl.hpp>

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
  auto [client, collection] = GetCDriverCollection();
  auto stats_ptr = statistics_->read[count_op.impl_->read_prefs_desc];

  MongoError error;
  stats::OperationStopwatch count_sw(stats_ptr,
                                     stats::ReadOperationStatistics::kCount);
  int64_t count = -1;
  if (count_op.impl_->use_new_count) {
    count = mongoc_collection_count_documents(
        collection.get(), count_op.impl_->filter.GetBson().get(),
        impl::GetNative(count_op.impl_->options),
        count_op.impl_->read_prefs.Get(), nullptr, error.GetNative());
  } else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"  // i know
    count = mongoc_collection_count_with_opts(
        collection.get(), MONGOC_QUERY_NONE,
        count_op.impl_->filter.GetBson().get(),  //
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
  auto [client, collection] = GetCDriverCollection();
  auto stats_ptr = statistics_->read[count_approx_op.impl_->read_prefs_desc];

  MongoError error;
  stats::OperationStopwatch count_approx_sw(
      stats_ptr, stats::ReadOperationStatistics::kCountApprox);
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
  auto [client, collection] = GetCDriverCollection();
  auto stats_ptr = statistics_->read[find_op.impl_->read_prefs_desc];

  impl::cdriver::CursorPtr cdriver_cursor(mongoc_collection_find_with_opts(
      collection.get(), find_op.impl_->filter.GetBson().get(),
      impl::GetNative(find_op.impl_->options),
      find_op.impl_->read_prefs.Get()));
  return Cursor(std::make_unique<impl::cdriver::CDriverCursorImpl>(
      std::move(client), std::move(cdriver_cursor), std::move(stats_ptr)));
}

WriteResult CDriverCollectionImpl::Execute(
    const operations::InsertOne& insert_op) {
  auto span = MakeSpan("mongo_insert_one");
  auto [client, collection] = GetCDriverCollection();
  auto stats_ptr = statistics_->write[insert_op.impl_->write_concern_desc];

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch insert_sw(
      stats_ptr, stats::WriteOperationStatistics::kInsertOne);
  if (mongoc_collection_insert_one(
          collection.get(), insert_op.impl_->document.GetBson().get(),
          impl::GetNative(insert_op.impl_->options), write_result.GetNative(),
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

  auto [client, collection] = GetCDriverCollection();
  auto stats_ptr = statistics_->write[insert_op.impl_->write_concern_desc];

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch insert_sw(
      stats_ptr, stats::WriteOperationStatistics::kInsertMany);
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
  auto [client, collection] = GetCDriverCollection();
  auto stats_ptr = statistics_->write[replace_op.impl_->write_concern_desc];

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch replace_sw(
      stats_ptr, stats::WriteOperationStatistics::kReplaceOne);
  if (mongoc_collection_replace_one(
          collection.get(), replace_op.impl_->selector.GetBson().get(),
          replace_op.impl_->replacement.GetBson().get(),
          impl::GetNative(replace_op.impl_->options), write_result.GetNative(),
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
  auto [client, collection] = GetCDriverCollection();
  auto stats_ptr = statistics_->write[update_op.impl_->write_concern_desc];

  bool should_retry_dupkey = update_op.impl_->should_retry_dupkey;
  while (true) {
    MongoError error;
    WriteResultHelper write_result;
    stats::OperationStopwatch<stats::WriteOperationStatistics> update_sw;
    bool has_succeeded = false;
    switch (update_op.impl_->mode) {
      case operations::Update::Mode::kSingle:
        update_sw.Reset(stats_ptr, stats::WriteOperationStatistics::kUpdateOne);
        has_succeeded = mongoc_collection_update_one(
            collection.get(), update_op.impl_->selector.GetBson().get(),
            update_op.impl_->update.GetBson().get(),
            impl::GetNative(update_op.impl_->options), write_result.GetNative(),
            error.GetNative());
        break;

      case operations::Update::Mode::kMulti:
        update_sw.Reset(stats_ptr,
                        stats::WriteOperationStatistics::kUpdateMany);
        has_succeeded = mongoc_collection_update_many(
            collection.get(), update_op.impl_->selector.GetBson().get(),
            update_op.impl_->update.GetBson().get(),
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
  auto [client, collection] = GetCDriverCollection();
  auto stats_ptr = statistics_->write[delete_op.impl_->write_concern_desc];

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch<stats::WriteOperationStatistics> delete_sw;
  bool has_succeeded = false;
  switch (delete_op.impl_->mode) {
    case operations::Delete::Mode::kSingle:
      delete_sw.Reset(stats_ptr, stats::WriteOperationStatistics::kDeleteOne);
      has_succeeded = mongoc_collection_delete_one(
          collection.get(), delete_op.impl_->selector.GetBson().get(),
          impl::GetNative(delete_op.impl_->options), write_result.GetNative(),
          error.GetNative());
      break;

    case operations::Delete::Mode::kMulti:
      delete_sw.Reset(stats_ptr, stats::WriteOperationStatistics::kDeleteMany);
      has_succeeded = mongoc_collection_delete_many(
          collection.get(), delete_op.impl_->selector.GetBson().get(),
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
  auto [client, collection] = GetCDriverCollection();
  auto stats_ptr = statistics_->write[fam_op.impl_->write_concern_desc];

  bool should_retry_dupkey = fam_op.impl_->should_retry_dupkey;
  while (true) {
    MongoError error;
    WriteResultHelper write_result;
    stats::OperationStopwatch fam_sw(
        stats_ptr, stats::WriteOperationStatistics::kFindAndModify);
    if (mongoc_collection_find_and_modify_with_opts(
            collection.get(), fam_op.impl_->query.GetBson().get(),
            fam_op.impl_->options.get(), write_result.GetNative(),
            error.GetNative())) {
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
  auto [client, collection] = GetCDriverCollection();
  auto stats_ptr = statistics_->write[fam_op.impl_->write_concern_desc];

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch fam_sw(
      stats_ptr, stats::WriteOperationStatistics::kFindAndRemove);
  if (mongoc_collection_find_and_modify_with_opts(
          collection.get(), fam_op.impl_->query.GetBson().get(),
          fam_op.impl_->options.get(), write_result.GetNative(),
          error.GetNative())) {
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

  auto client = GetCDriverClient();
  mongoc_bulk_operation_set_client(bulk_op.impl_->bulk.get(), client.get());

  auto stats_ptr = statistics_->write[bulk_op.impl_->write_concern_desc];

  MongoError error;
  WriteResultHelper write_result;
  stats::OperationStopwatch bulk_sw(stats_ptr,
                                    stats::WriteOperationStatistics::kBulk);
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
  auto [client, collection] = GetCDriverCollection();
  // TODO: this is not quite correct for aggregations with "$out"/"$merge"
  auto stats_ptr = statistics_->read[aggregate_op.impl_->read_prefs_desc];

  auto pipeline_doc = aggregate_op.impl_->pipeline.GetInternalArrayDocument();
  impl::cdriver::CursorPtr cdriver_cursor(mongoc_collection_aggregate(
      collection.get(), MONGOC_QUERY_NONE, pipeline_doc.GetBson().get(),
      impl::GetNative(aggregate_op.impl_->options),
      aggregate_op.impl_->read_prefs.Get()));
  return Cursor(std::make_unique<impl::cdriver::CDriverCursorImpl>(
      std::move(client), std::move(cdriver_cursor), std::move(stats_ptr)));
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

}  // namespace storages::mongo::impl::cdriver
