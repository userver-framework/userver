#pragma once

#include <memory>
#include <string>

#include <mongocxx/collection.hpp>

#include "pool_impl.hpp"

namespace storages {
namespace mongo {
namespace impl {

class CollectionImpl {
 public:
  CollectionImpl(std::shared_ptr<PoolImpl> pool, std::string database_name,
                 std::string collection_name)
      : pool_(std::move(pool)),
        database_name_(std::move(database_name)),
        collection_name_(std::move(collection_name)) {}

  PoolImpl& GetPool() { return *pool_; }

  mongocxx::collection GetCollection(const PoolImpl::ConnectionPtr& conn) {
    return conn->database(database_name_).collection(collection_name_);
  }

 public:
  const std::shared_ptr<PoolImpl> pool_;
  const std::string database_name_;
  const std::string collection_name_;
};

}  // namespace impl
}  // namespace mongo
}  // namespace storages
