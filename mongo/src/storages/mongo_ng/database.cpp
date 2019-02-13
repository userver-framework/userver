#include <storages/mongo_ng/database.hpp>

#include <storages/mongo_ng/exception.hpp>
#include <utils/text.hpp>

#include <storages/mongo_ng/bson_error.hpp>
#include <storages/mongo_ng/collection_impl.hpp>
#include <storages/mongo_ng/wrappers.hpp>

namespace storages::mongo_ng::impl {

Database::Database(PoolImplPtr pool, std::string database_name)
    : pool_(std::move(pool)), database_name_(std::move(database_name)) {
  if (!utils::text::IsCString(database_name_)) {
    throw MongoException("Invalid database name: '" + database_name);
  }
}

bool Database::HasCollection(const std::string& collection_name) const {
  if (!utils::text::IsCString(collection_name)) {
    throw MongoException("Invalid collection name: '" + collection_name + '\'');
  }

  auto client = pool_->Acquire();
  DatabasePtr database(
      mongoc_client_get_database(client.get(), database_name_.c_str()));

  BsonError error;
  bool has_collection = mongoc_database_has_collection(
      database.get(), collection_name.c_str(), error.Get());
  if (error) error.Throw();
  return has_collection;
}

Collection Database::GetCollection(std::string collection_name) const {
  return Collection(std::make_shared<CollectionImpl>(
      pool_, database_name_, std::move(collection_name)));
}

}  // namespace storages::mongo_ng::impl
