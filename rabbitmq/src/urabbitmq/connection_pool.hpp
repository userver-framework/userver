#pragma once

#include <memory>

#include <boost/lockfree/queue.hpp>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/utils/periodic_task.hpp>

#include <userver/urabbitmq/client_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class ConnectionPtr;
class Connection;

namespace statistics {
class ConnectionStatistics;
}

class ConnectionPool : public std::enable_shared_from_this<ConnectionPool> {
 public:
  static std::shared_ptr<ConnectionPool> Create(
      clients::dns::Resolver& resolver, const EndpointInfo& endpoint_info,
      const AuthSettings& auth_settings, const PoolSettings& pool_settings,
      bool use_secure_connection, statistics::ConnectionStatistics& stats);
  ~ConnectionPool();

  ConnectionPtr Acquire(engine::Deadline deadline);
  void Release(std::unique_ptr<Connection> connection);

  void NotifyConnectionAdopted();

 protected:
  ConnectionPool(clients::dns::Resolver& resolver,
                 const EndpointInfo& endpoint_info,
                 const AuthSettings& auth_settings,
                 const PoolSettings& pool_settings, bool use_secure_connection,
                 statistics::ConnectionStatistics& stats);

 private:
  std::unique_ptr<Connection> Pop(engine::Deadline deadline);
  std::unique_ptr<Connection> TryPop();

  void PushConnection(engine::Deadline deadline);
  std::unique_ptr<Connection> CreateConnection(engine::Deadline deadline);
  void Drop(Connection* connection) noexcept;

  void RunMonitor();

  clients::dns::Resolver& resolver_;
  const EndpointInfo endpoint_info_;
  const AuthSettings auth_settings_;
  const PoolSettings pool_settings_;
  bool use_secure_connection_;
  statistics::ConnectionStatistics& stats_;

  engine::Semaphore given_away_semaphore_;
  engine::Semaphore connecting_semaphore_;
  boost::lockfree::queue<Connection*> queue_;
  std::atomic<size_t> size_{0};

  utils::PeriodicTask monitor_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
