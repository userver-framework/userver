#pragma once

#include <memory>

#include <boost/lockfree/queue.hpp>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/semaphore.hpp>

#include <storages/clickhouse/impl/settings.hpp>
#include <storages/clickhouse/stats/pool_statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

class Connection;
class ConnectionPtr;

class PoolImpl final : public std::enable_shared_from_this<PoolImpl> {
 public:
  PoolImpl(clients::dns::Resolver&, PoolSettings&& settings);
  ~PoolImpl();

  ConnectionPtr Acquire();
  void Release(Connection*);

  stats::PoolStatistics& GetStatistics();

  const std::string& GetHostName() const;

 private:
  Connection* Pop();
  Connection* TryPop();

  Connection* Create();
  void Drop(Connection*);

  const PoolSettings pool_settings_;

  engine::Semaphore given_away_semaphore_;
  engine::Semaphore connecting_semaphore_;
  boost::lockfree::queue<Connection*> queue_;

  clients::dns::Resolver& resolver_;

  stats::PoolStatistics statistics_;
};

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
