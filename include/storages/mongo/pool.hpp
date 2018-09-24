#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include <storages/mongo/collection.hpp>
#include <storages/mongo/mongo.hpp>

namespace engine {
class TaskProcessor;
}  // namespace engine

namespace storages {
namespace mongo {

namespace impl {
class PoolImpl;
}  // namespace impl

constexpr size_t kCriticalPoolSize = 1000;

class PoolError : public MongoError {
  using MongoError::MongoError;
};

class InvalidConfig : public PoolError {
  using PoolError::PoolError;
};

class Pool {
 public:
  Pool(const std::string& uri, engine::TaskProcessor& task_processor,
       int conn_timeout_ms, int so_timeout_ms, size_t min_size,
       size_t max_size);
  ~Pool();

  Collection GetCollection(std::string collection_name);
  Collection GetCollection(std::string database_name,
                           std::string collection_name);

 private:
  std::shared_ptr<impl::PoolImpl> impl_;
};

using PoolPtr = std::shared_ptr<Pool>;

}  // namespace mongo
}  // namespace storages
