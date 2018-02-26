#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

#include <mongo/client/dbclient.h>

#include <engine/task/task_processor.hpp>

namespace storages {
namespace mongo {

constexpr bool kAutoReconnect = true;
constexpr size_t kCriticalPoolSize = 1000;

class PoolError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class InvalidConfig : public PoolError {
  using PoolError::PoolError;
};

class CollectionWrapper;

class Pool : public std::enable_shared_from_this<Pool> {
 public:
  Pool(const std::string& uri, engine::TaskProcessor& task_processor,
       int so_timeout_ms, size_t min_size, size_t max_size);
  ~Pool();

  CollectionWrapper GetCollection(const std::string& collection_name);

  using ConnectionPtr =
      std::unique_ptr<::mongo::DBClientConnection,
                      std::function<void(::mongo::DBClientConnection*)>>;

 private:
  friend class CollectionWrapper;

  ConnectionPtr Acquire();
  void Release(ConnectionPtr::pointer client);

  class Impl;
  const std::unique_ptr<Impl> impl_;
};

using PoolPtr = std::shared_ptr<Pool>;

}  // namespace mongo
}  // namespace storages
