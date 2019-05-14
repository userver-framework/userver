#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>

#include <mongoc/mongoc.h>

#include <boost/lockfree/queue.hpp>

#include <engine/deadline.hpp>
#include <engine/semaphore.hpp>
#include <storages/mongo/pool_config.hpp>
#include <storages/mongo/stats.hpp>
#include <storages/mongo/wrappers.hpp>
#include <utils/assert.hpp>

namespace storages::mongo::impl {

class PoolImpl {
 public:
  class ClientPusher {
   public:
    explicit ClientPusher(PoolImpl* pool) noexcept : pool_(pool) {
      UASSERT(pool_);
    }
    void operator()(mongoc_client_t* client) const noexcept {
      pool_->Push(client);
    }

   private:
    PoolImpl* pool_;
  };
  using BoundClientPtr = std::unique_ptr<mongoc_client_t, ClientPusher>;

  PoolImpl(std::string id, const std::string& uri_string,
           const PoolConfig& config);
  ~PoolImpl();

  size_t SizeApprox() const;
  size_t MaxSize() const;
  const std::string& DefaultDatabaseName() const;

  stats::PoolStatistics& GetStatistics();

  BoundClientPtr Acquire();

 private:
  void Push(mongoc_client_t*) noexcept;
  mongoc_client_t* Pop();

  mongoc_client_t* TryGetIdle();
  mongoc_client_t* Create();

  const std::string id_;
  const std::string app_name_;
  std::string default_database_;
  UriPtr uri_;
  mongoc_ssl_opt_t ssl_opt_{};

  const size_t max_size_;
  const std::chrono::milliseconds queue_timeout_;
  engine::Semaphore size_semaphore_;
  engine::Semaphore connecting_semaphore_;
  boost::lockfree::queue<mongoc_client_t*> queue_;

  stats::PoolStatistics statistics_;
};

using BoundClientPtr = PoolImpl::BoundClientPtr;
using PoolImplPtr = std::shared_ptr<PoolImpl>;

}  // namespace storages::mongo::impl
