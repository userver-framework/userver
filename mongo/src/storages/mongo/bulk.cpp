#include <storages/mongo/bulk.hpp>

#include <mongoc/mongoc.h>

#include <storages/mongo/mongo_error.hpp>

#include <storages/mongo/bulk_ops_impl.hpp>
#include <storages/mongo/operations_common.hpp>
#include <storages/mongo/operations_impl.hpp>

namespace storages::mongo::operations {

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Bulk::Bulk(Mode mode) {
  const bool is_ordered = (mode == Mode::kOrdered);
  impl_->bulk.reset(mongoc_bulk_operation_new(is_ordered));
}

Bulk::~Bulk() = default;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Bulk::Bulk(Bulk&&) noexcept = default;
Bulk& Bulk::operator=(Bulk&&) noexcept = default;

void Bulk::SetOption(options::WriteConcern::Level level) {
  mongoc_bulk_operation_set_write_concern(impl_->bulk.get(),
                                          impl::MakeWriteConcern(level).get());
}

void Bulk::SetOption(const options::WriteConcern& write_concern) {
  mongoc_bulk_operation_set_write_concern(
      impl_->bulk.get(), impl::MakeWriteConcern(write_concern).get());
}

void Bulk::SetOption(options::SuppressServerExceptions) {
  impl_->should_throw = false;
}

void Bulk::Append(const bulk_ops::InsertOne& insert_subop) {
  MongoError error;
  if (!mongoc_bulk_operation_insert_with_opts(
          impl_->bulk.get(), insert_subop.impl_->document.GetBson().get(),
          nullptr, error.GetNative())) {
    error.Throw("Error appending insert to bulk");
  }
}

void Bulk::Append(const bulk_ops::ReplaceOne& replace_subop) {
  MongoError error;
  if (!mongoc_bulk_operation_replace_one_with_opts(
          impl_->bulk.get(), replace_subop.impl_->selector.GetBson().get(),
          replace_subop.impl_->replacement.GetBson().get(),
          impl::GetNative(replace_subop.impl_->options), error.GetNative())) {
    error.Throw("Error appending replace to bulk");
  }
}

void Bulk::Append(const bulk_ops::Update& update_subop) {
  MongoError error;
  bool has_succeeded = false;
  switch (update_subop.impl_->mode) {
    case bulk_ops::Update::Mode::kSingle:
      has_succeeded = mongoc_bulk_operation_update_one_with_opts(
          impl_->bulk.get(), update_subop.impl_->selector.GetBson().get(),
          update_subop.impl_->update.GetBson().get(),
          impl::GetNative(update_subop.impl_->options), error.GetNative());
      break;

    case bulk_ops::Update::Mode::kMulti:
      has_succeeded = mongoc_bulk_operation_update_many_with_opts(
          impl_->bulk.get(), update_subop.impl_->selector.GetBson().get(),
          update_subop.impl_->update.GetBson().get(),
          impl::GetNative(update_subop.impl_->options), error.GetNative());
      break;
  }
  if (!has_succeeded) error.Throw("Error appending update to bulk");
}

void Bulk::Append(const bulk_ops::Delete& delete_subop) {
  MongoError error;
  bool has_succeeded = false;
  switch (delete_subop.impl_->mode) {
    case bulk_ops::Delete::Mode::kSingle:
      has_succeeded = mongoc_bulk_operation_remove_one_with_opts(
          impl_->bulk.get(), delete_subop.impl_->selector.GetBson().get(),
          nullptr, error.GetNative());
      break;

    case bulk_ops::Delete::Mode::kMulti:
      has_succeeded = mongoc_bulk_operation_remove_many_with_opts(
          impl_->bulk.get(), delete_subop.impl_->selector.GetBson().get(),
          nullptr, error.GetNative());
      break;
  }
  if (!has_succeeded) error.Throw("Error appending delete to bulk");
}

}  // namespace storages::mongo::operations
