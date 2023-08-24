#pragma once

#include <string>

#include <userver/storages/mongo/collection.hpp>

#include <storages/mongo/pool_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl {

class Database {
 public:
  Database(PoolImplPtr pool, std::string database_name);

  void DropDatabase();

  bool HasCollection(const std::string& collection_name) const;

  Collection GetCollection(std::string collection_name) const;

 private:
  PoolImplPtr pool_;
  std::string database_name_;
};

}  // namespace storages::mongo::impl

USERVER_NAMESPACE_END
