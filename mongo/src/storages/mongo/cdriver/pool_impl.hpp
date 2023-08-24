#pragma once

#include <chrono>

#include <mongoc/mongoc.h>
#include <boost/lockfree/queue.hpp>

#include <storages/mongo/cdriver/async_stream.hpp>
#include <storages/mongo/cdriver/wrappers.hpp>
#include <storages/mongo/dynamic_config.hpp>
#include <storages/mongo/pool_impl.hpp>
#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/storages/mongo/pool_config.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::impl::cdriver {

class CDriverPoolImpl final : public PoolImpl {
 public:
  class ClientPusher {
   public:
    explicit ClientPusher(CDriverPoolImpl* pool) noexcept : pool_(pool) {
      UASSERT(pool_);
    }
    void operator()(mongoc_client_t* client) const noexcept {
      pool_->Push(client);
    }

   private:
    CDriverPoolImpl* pool_;
  };
  using BoundClientPtr = std::unique_ptr<mongoc_client_t, ClientPusher>;

  CDriverPoolImpl(std::string id, const std::string& uri_string,
                  const PoolConfig& config,
                  clients::dns::Resolver* dns_resolver,
                  dynamic_config::Source config_source);
  ~CDriverPoolImpl() override;

  const std::string& DefaultDatabaseName() const override;

  void Ping() override;

  size_t InUseApprox() const override;
  size_t SizeApprox() const override;
  size_t MaxSize() const override;
  void SetMaxSize(size_t max_size) override;

  /// @throws CancelledException, PoolOverloadException
  BoundClientPtr Acquire();

 private:
  mongoc_client_t* Pop();
  void Push(mongoc_client_t*) noexcept;
  void Drop(mongoc_client_t*) noexcept;

  mongoc_client_t* TryGetIdle();
  mongoc_client_t* Create();

  void DoMaintenance();

  const std::string app_name_;
  std::string default_database_;
  UriPtr uri_;
  AsyncStreamInitiatorData init_data_;

  std::atomic<size_t> max_size_;
  const size_t idle_limit_;
  const std::chrono::milliseconds queue_timeout_;
  std::atomic<size_t> size_;
  engine::Semaphore in_use_semaphore_;
  engine::Semaphore connecting_semaphore_;
  boost::lockfree::queue<mongoc_client_t*> queue_;
  utils::PeriodicTask maintenance_task_;
};

}  // namespace storages::mongo::impl::cdriver

USERVER_NAMESPACE_END
