#include <userver/storages/mongo/bulk.hpp>

#include <mongoc/mongoc.h>

#include <userver/storages/mongo/mongo_error.hpp>

#include <storages/mongo/bulk_ops_impl.hpp>
#include <storages/mongo/operations_common.hpp>
#include <storages/mongo/operations_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::operations {
namespace {

auto EnsureBulk(impl::cdriver::BulkOperationPtr& bulk_ptr, Bulk::Mode mode) {
  if (!bulk_ptr) {
    const bool is_ordered = (mode == Bulk::Mode::kOrdered);
    bulk_ptr.reset(mongoc_bulk_operation_new(is_ordered));
  }
  return bulk_ptr.get();
}

}  // namespace

Bulk::Bulk(Mode mode) : impl_(mode) {}
Bulk::~Bulk() = default;

Bulk::Bulk(Bulk&&) noexcept = default;
Bulk& Bulk::operator=(Bulk&&) noexcept = default;

bool Bulk::IsEmpty() const { return !impl_->bulk; }

void Bulk::SetOption(options::WriteConcern::Level level) {
  mongoc_bulk_operation_set_write_concern(
      EnsureBulk(impl_->bulk, impl_->mode),
      impl::MakeCDriverWriteConcern(level).get());
}

void Bulk::SetOption(const options::WriteConcern& write_concern) {
  mongoc_bulk_operation_set_write_concern(
      EnsureBulk(impl_->bulk, impl_->mode),
      impl::MakeCDriverWriteConcern(write_concern).get());
}

void Bulk::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

void Bulk::Append(const bulk_ops::InsertOne& insert_subop) {
  MongoError error;
  const bson_t* native_bson_ptr = insert_subop.impl_->document.GetBson().get();
  if (!mongoc_bulk_operation_insert_with_opts(
          EnsureBulk(impl_->bulk, impl_->mode), native_bson_ptr, nullptr,
          error.GetNative())) {
    error.Throw("Error appending insert to bulk");
  }
}

void Bulk::Append(const bulk_ops::ReplaceOne& replace_subop) {
  MongoError error;
  const bson_t* native_selector_bson_ptr =
      replace_subop.impl_->selector.GetBson().get();
  const bson_t* native_replacement_bson_ptr =
      replace_subop.impl_->replacement.GetBson().get();
  if (!mongoc_bulk_operation_replace_one_with_opts(
          EnsureBulk(impl_->bulk, impl_->mode), native_selector_bson_ptr,
          native_replacement_bson_ptr,
          impl::GetNative(replace_subop.impl_->options), error.GetNative())) {
    error.Throw("Error appending replace to bulk");
  }
}

void Bulk::Append(const bulk_ops::Update& update_subop) {
  MongoError error;
  const bson_t* native_selector_bson_ptr =
      update_subop.impl_->selector.GetBson().get();
  const bson_t* native_update_bson_ptr =
      update_subop.impl_->update.GetBson().get();
  bool has_succeeded = false;
  switch (update_subop.impl_->mode) {
    case bulk_ops::Update::Mode::kSingle:
      has_succeeded = mongoc_bulk_operation_update_one_with_opts(
          EnsureBulk(impl_->bulk, impl_->mode), native_selector_bson_ptr,
          native_update_bson_ptr, impl::GetNative(update_subop.impl_->options),
          error.GetNative());
      break;

    case bulk_ops::Update::Mode::kMulti:
      has_succeeded = mongoc_bulk_operation_update_many_with_opts(
          EnsureBulk(impl_->bulk, impl_->mode), native_selector_bson_ptr,
          native_update_bson_ptr, impl::GetNative(update_subop.impl_->options),
          error.GetNative());
      break;
  }
  if (!has_succeeded) error.Throw("Error appending update to bulk");
}

void Bulk::Append(const bulk_ops::Delete& delete_subop) {
  MongoError error;
  const bson_t* native_selector_bson_ptr =
      delete_subop.impl_->selector.GetBson().get();
  bool has_succeeded = false;
  switch (delete_subop.impl_->mode) {
    case bulk_ops::Delete::Mode::kSingle:
      has_succeeded = mongoc_bulk_operation_remove_one_with_opts(
          EnsureBulk(impl_->bulk, impl_->mode), native_selector_bson_ptr,
          nullptr, error.GetNative());
      break;

    case bulk_ops::Delete::Mode::kMulti:
      has_succeeded = mongoc_bulk_operation_remove_many_with_opts(
          EnsureBulk(impl_->bulk, impl_->mode), native_selector_bson_ptr,
          nullptr, error.GetNative());
      break;
  }
  if (!has_succeeded) error.Throw("Error appending delete to bulk");
}

}  // namespace storages::mongo::operations

USERVER_NAMESPACE_END
