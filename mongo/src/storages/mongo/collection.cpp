#include <userver/storages/mongo/collection.hpp>

#include <userver/tracing/span.hpp>

#include <storages/mongo/collection_impl.hpp>

namespace storages::mongo {
Collection::Collection(std::shared_ptr<impl::CollectionImpl> impl)
    : impl_(std::move(impl)) {}

size_t Collection::Execute(const operations::Count& count_op) const {
  return impl_->Execute(count_op);
}

size_t Collection::Execute(
    const operations::CountApprox& count_approx_op) const {
  return impl_->Execute(count_approx_op);
}

Cursor Collection::Execute(const operations::Find& find_op) const {
  return impl_->Execute(find_op);
}

WriteResult Collection::Execute(const operations::InsertOne& insert_op) {
  return impl_->Execute(insert_op);
}

WriteResult Collection::Execute(const operations::InsertMany& insert_op) {
  return impl_->Execute(insert_op);
}

WriteResult Collection::Execute(const operations::ReplaceOne& replace_op) {
  return impl_->Execute(replace_op);
}

WriteResult Collection::Execute(const operations::Update& update_op) {
  return impl_->Execute(update_op);
}

WriteResult Collection::Execute(const operations::Delete& delete_op) {
  return impl_->Execute(delete_op);
}

WriteResult Collection::Execute(const operations::FindAndModify& fam_op) {
  return impl_->Execute(fam_op);
}

WriteResult Collection::Execute(const operations::FindAndRemove& fam_op) {
  return impl_->Execute(fam_op);
}

WriteResult Collection::Execute(operations::Bulk&& bulk_op) {
  return impl_->Execute(std::move(bulk_op));
}

Cursor Collection::Execute(const operations::Aggregate& aggregate_op) {
  return impl_->Execute(aggregate_op);
}

std::optional<std::string> Collection::GetCurrentSpanLink() {
  auto* span = tracing::Span::CurrentSpanUnchecked();
  if (span) return span->GetLink();
  return std::nullopt;
}

}  // namespace storages::mongo
