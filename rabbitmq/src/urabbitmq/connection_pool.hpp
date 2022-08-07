#pragma once

#include <memory>

#include <boost/lockfree/queue.hpp>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/utils/periodic_task.hpp>

#include <userver/urabbitmq/client_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class ConnectionPtr;
class Connection;

namespace statistics {
class ConnectionStatistics;
}

struct PoolSettings final {
  size_t min_pool_size;

  size_t max_pool_size;

  bool secure;
};

class ConnectionPool : public std::enable_shared_from_this<ConnectionPool> {
 public:
  static std::shared_ptr<ConnectionPool> Create(
      clients::dns::Resolver& resolver, const EndpointInfo& endpoint_info,
      const AuthSettings& auth_settings, const PoolSettings& pool_settings,
      statistics::ConnectionStatistics& stats);
  ~ConnectionPool();

  ConnectionPtr Acquire();
  void Release(std::unique_ptr<Connection> conn);

  void NotifyConnectionAdopted();

 protected:
  ConnectionPool(clients::dns::Resolver& resolver,
                 const EndpointInfo& endpoint_info,
                 const AuthSettings& auth_settings,
                 const PoolSettings& pool_settings,
                 statistics::ConnectionStatistics& stats);

 private:
  std::unique_ptr<Connection> Pop();
  std::unique_ptr<Connection> TryPop();

  void PushConnection();
  std::unique_ptr<Connection> Create();
  void Drop(Connection* conn) noexcept;

  void RunMonitor();

  clients::dns::Resolver& resolver_;
  const EndpointInfo endpoint_info_;
  const AuthSettings auth_settings_;
  const PoolSettings pool_settings_;
  statistics::ConnectionStatistics& stats_;

  boost::lockfree::queue<Connection*> queue_;
  std::atomic<size_t> size_{0};

  utils::PeriodicTask monitor_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END