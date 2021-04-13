#pragma once

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
#include <utils/periodic_task.hpp>

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

  size_t InUseApprox() const;
  size_t SizeApprox() const;
  size_t MaxSize() const;
  const std::string& DefaultDatabaseName() const;

  const stats::PoolStatistics& GetStatistics() const;
  stats::PoolStatistics& GetStatistics();

  BoundClientPtr Acquire();

 private:
  mongoc_client_t* Pop();
  void Push(mongoc_client_t*) noexcept;
  void Drop(mongoc_client_t*) noexcept;

  mongoc_client_t* TryGetIdle();
  mongoc_client_t* Create();

  void DoMaintenance();

  const std::string id_;
  const std::string app_name_;
  std::string default_database_;
  UriPtr uri_;
  mongoc_ssl_opt_t ssl_opt_{};

  const size_t max_size_;
  const size_t idle_limit_;
  const std::chrono::milliseconds queue_timeout_;
  std::atomic<size_t> size_;
  engine::Semaphore in_use_semaphore_;
  engine::Semaphore connecting_semaphore_;
  boost::lockfree::queue<mongoc_client_t*> queue_;
  utils::PeriodicTask maintenance_task_;

  stats::PoolStatistics statistics_;
};

using BoundClientPtr = PoolImpl::BoundClientPtr;
using PoolImplPtr = std::shared_ptr<PoolImpl>;

}  // namespace storages::mongo::impl
