#pragma once

#include <string>

#include <storages/mongo_ng/collection.hpp>

#include <storages/mongo_ng/pool_impl.hpp>

namespace storages::mongo_ng::impl {

class Database {
 public:
  Database(PoolImplPtr pool, std::string database_name);

  bool HasCollection(const std::string& collection_name) const;

  Collection GetCollection(std::string collection_name) const;

 private:
  PoolImplPtr pool_;
  std::string database_name_;
};

}  // namespace storages::mongo_ng::impl
