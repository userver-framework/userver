#pragma once

#include <mongoc/mongoc.h>

#include <tuple>

#include <storages/mongo_ng/exception.hpp>
#include <utils/text.hpp>

#include <storages/mongo_ng/pool_impl.hpp>
#include <storages/mongo_ng/wrappers.hpp>

namespace storages::mongo_ng::impl {

class CollectionImpl {
 public:
  CollectionImpl(PoolImplPtr pool_impl, std::string database_name,
                 std::string collection_name)
      : pool_impl_(std::move(pool_impl)),
        database_name_(std::move(database_name)),
        collection_name_(std::move(collection_name)) {
    if (!utils::text::IsCString(database_name_)) {
      throw MongoException("Invalid database name: '" + database_name_ + '\'');
    }
    if (!utils::text::IsCString(collection_name_)) {
      throw MongoException("Invalid collection name: '" + collection_name_ +
                           '\'');
    }
  }

  std::tuple<BoundClientPtr, CollectionPtr> GetNativeCollection() {
    auto client = pool_impl_->Acquire();
    CollectionPtr collection(mongoc_client_get_collection(
        client.get(), database_name_.c_str(), collection_name_.c_str()));
    return std::make_tuple(std::move(client), std::move(collection));
  }

 private:
  PoolImplPtr pool_impl_;
  std::string database_name_;
  std::string collection_name_;
};

}  // namespace storages::mongo_ng::impl
