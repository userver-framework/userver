#include <storages/mongo_ng/collection.hpp>

#include <storages/mongo_ng/bson_error.hpp>
#include <storages/mongo_ng/collection_impl.hpp>
#include <storages/mongo_ng/cursor_impl.hpp>
#include <storages/mongo_ng/operations_impl.hpp>

namespace storages::mongo_ng {
namespace {

const bson_t* GetNative(
    const boost::optional<formats::bson::impl::BsonBuilder>& builder) {
  return builder ? builder->Get() : nullptr;
}

}  // namespace

Collection::Collection(std::shared_ptr<impl::CollectionImpl> impl)
    : impl_(std::move(impl)) {}

void Collection::InsertOne(const formats::bson::Document& document) {
  auto [client, collection] = impl_->GetNativeCollection();

  impl::BsonError error;
  if (!mongoc_collection_insert_one(collection.get(), document.GetBson().get(),
                                    nullptr, nullptr, error.Get())) {
    error.Throw();
  }
}

size_t Collection::Execute(const operations::Count& count_op) const {
  auto [client, collection] = impl_->GetNativeCollection();

  impl::BsonError error;
  auto count = mongoc_collection_count_documents(
      collection.get(), count_op.impl_->filter.GetBson().get(),
      GetNative(count_op.impl_->options), count_op.impl_->read_prefs.get(),
      nullptr, error.Get());
  if (count < 0) error.Throw();
  return count;
}

size_t Collection::Execute(
    const operations::CountApprox& count_approx_op) const {
  auto [client, collection] = impl_->GetNativeCollection();

  impl::BsonError error;
  auto count = mongoc_collection_estimated_document_count(
      collection.get(), GetNative(count_approx_op.impl_->options),
      count_approx_op.impl_->read_prefs.get(), nullptr, error.Get());
  if (count < 0) error.Throw();
  return count;
}

Cursor Collection::Execute(const operations::Find& find_op) const {
  auto [client, collection] = impl_->GetNativeCollection();
  impl::CursorPtr native_cursor(mongoc_collection_find_with_opts(
      collection.get(), find_op.impl_->filter.GetBson().get(),
      GetNative(find_op.impl_->options), find_op.impl_->read_prefs.get()));
  return Cursor(impl::CursorImpl(std::move(client), std::move(native_cursor)));
}

}  // namespace storages::mongo_ng
