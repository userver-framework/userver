#include <storages/mongo/database.hpp>

#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/mongo_error.hpp>
#include <userver/utils/text.hpp>

#include <formats/bson/wrappers.hpp>
#include <storages/mongo/cdriver/collection_impl.hpp>
#include <storages/mongo/cdriver/pool_impl.hpp>
#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/collection_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

Database::Database(PoolImplPtr pool, std::string database_name)
    : pool_(std::move(pool)), database_name_(std::move(database_name)) {
  if (!utils::text::IsCString(database_name_)) {
    throw MongoException("Invalid database name: '" + database_name_);
  }
}

void Database::DropDatabase() {
  auto* pool = dynamic_cast<cdriver::CDriverPoolImpl*>(pool_.get());
  UASSERT(pool);
  auto client = pool->Acquire();
  const cdriver::DatabasePtr database(
      mongoc_client_get_database(client.get(), database_name_.c_str()));

  MongoError error;
  mongoc_database_drop(database.get(), error.GetNative());
  if (error) {
    error.Throw("Error dropping the database");
  }
}

bool Database::HasCollection(const std::string& collection_name) const {
  if (!utils::text::IsCString(collection_name)) {
    throw MongoException("Invalid collection name: '" + collection_name + '\'');
  }

  auto* pool = dynamic_cast<cdriver::CDriverPoolImpl*>(pool_.get());
  UASSERT(pool);
  auto client = pool->Acquire();
  const cdriver::DatabasePtr database(
      mongoc_client_get_database(client.get(), database_name_.c_str()));

  MongoError error;
  const bool has_collection = mongoc_database_has_collection(
      database.get(), collection_name.c_str(), error.GetNative());
  if (error) {
    error.Throw("Error checking for collection existence");
  }
  return has_collection;
}

Collection Database::GetCollection(std::string collection_name) const {
  return Collection(std::make_shared<cdriver::CDriverCollectionImpl>(
      pool_, database_name_, std::move(collection_name)));
}

std::vector<std::string> Database::ListCollectionNames() const {
  auto* pool = dynamic_cast<cdriver::CDriverPoolImpl*>(pool_.get());
  UASSERT(pool);
  auto client = pool->Acquire();
  const cdriver::DatabasePtr database(
      mongoc_client_get_database(client.get(), database_name_.c_str()));

  MongoError error;
  formats::bson::impl::RawPtr<char*> collection_names(
      mongoc_database_get_collection_names_with_opts(database.get(), nullptr,
                                                     error.GetNative()));
  if (error) {
    error.Throw("Error listing existing collections");
  }

  auto raw_collection_names = collection_names.get();
  UASSERT(raw_collection_names);

  std::vector<std::string> collections;
  while (*raw_collection_names) {
    formats::bson::impl::RawPtr<char> collection_name(*raw_collection_names);
    collections.emplace_back(collection_name.get());
    ++raw_collection_names;
  }

  return collections;
}

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
