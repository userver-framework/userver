#pragma once

#include <atomic>
#include <cassert>
#include <memory>
#include <string>

#include <mongoc/mongoc.h>

#include <boost/lockfree/queue.hpp>

#include <storages/mongo_ng/pool_config.hpp>
#include <storages/mongo_ng/wrappers.hpp>

namespace storages::mongo_ng::impl {

class PoolImpl {
 public:
  class ClientPusher {
   public:
    explicit ClientPusher(PoolImpl* pool) : pool_(pool) { assert(pool_); }
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

  const std::string& DefaultDatabaseName() const;

  BoundClientPtr Acquire();

 private:
  void Push(mongoc_client_t*) noexcept;
  mongoc_client_t* Pop();

  mongoc_client_t* Create();

  const std::string id_;
  const std::string app_name_;
  std::string default_database_;
  UriPtr uri_;
  mongoc_ssl_opt_t ssl_opt_;
  boost::lockfree::queue<mongoc_client_t*> queue_;
  std::atomic<size_t> size_;
};

using BoundClientPtr = PoolImpl::BoundClientPtr;
using PoolImplPtr = std::shared_ptr<PoolImpl>;

}  // namespace storages::mongo_ng::impl
