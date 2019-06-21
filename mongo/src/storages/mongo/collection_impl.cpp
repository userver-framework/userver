#include <storages/mongo/collection_impl.hpp>

#include <mongoc/mongoc.h>

#include <storages/mongo/exception.hpp>
#include <tracing/tags.hpp>
#include <utils/text.hpp>

#include <storages/mongo/wrappers.hpp>

namespace storages::mongo::impl {

CollectionImpl::CollectionImpl(PoolImplPtr pool_impl, std::string database_name,
                               std::string collection_name)
    : pool_impl_(std::move(pool_impl)),
      database_name_(std::move(database_name)),
      collection_name_(std::move(collection_name)),
      statistics_(pool_impl_->GetStatistics().collections[collection_name_]) {
  if (!utils::text::IsCString(database_name_)) {
    throw MongoException("Invalid database name: '" + database_name_ + '\'');
  }
  if (!utils::text::IsCString(collection_name_)) {
    throw MongoException("Invalid collection name: '" + collection_name_ +
                         '\'');
  }
}

const std::string& CollectionImpl::GetDatabaseName() const {
  return database_name_;
}
const std::string& CollectionImpl::GetCollectionName() const {
  return collection_name_;
}

BoundClientPtr CollectionImpl::GetClient() { return pool_impl_->Acquire(); }

std::tuple<BoundClientPtr, CollectionPtr>
CollectionImpl::GetNativeCollection() {
  auto client = GetClient();
  CollectionPtr collection(mongoc_client_get_collection(
      client.get(), database_name_.c_str(), collection_name_.c_str()));
  return std::make_tuple(std::move(client), std::move(collection));
}

tracing::Span CollectionImpl::MakeSpan(const std::string& name) const {
  tracing::Span span(name);
  span.AddTag(tracing::kDatabaseType, tracing::kDatabaseMongoType);
  span.AddTag(tracing::kDatabaseInstance, database_name_);
  span.AddTag(tracing::kDatabaseCollection, collection_name_);
  return span;
}

stats::CollectionStatistics& CollectionImpl::GetCollectionStatistics() {
  return *statistics_;
}

}  // namespace storages::mongo::impl
