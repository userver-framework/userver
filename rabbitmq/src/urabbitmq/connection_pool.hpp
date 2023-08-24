#pragma once

#include <memory>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/drivers/impl/connection_pool_base.hpp>
#include <userver/utils/periodic_task.hpp>

#include <userver/urabbitmq/client_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class ConnectionPtr;
class Connection;

namespace statistics {
class ConnectionStatistics;
}

class ConnectionPool final
    : public drivers::impl::ConnectionPoolBase<Connection, ConnectionPool> {
 public:
  static std::shared_ptr<ConnectionPool> Create(
      clients::dns::Resolver& resolver, const EndpointInfo& endpoint_info,
      const AuthSettings& auth_settings, const PoolSettings& pool_settings,
      bool use_secure_connection, statistics::ConnectionStatistics& stats);
  ~ConnectionPool();

  ConnectionPtr Acquire(engine::Deadline deadline);
  void Release(std::unique_ptr<Connection> connection);

  void NotifyConnectionAdopted();

  // This should be protected in perfect world, but it gets too cumbersome and a
  // bit bloated; this is private for the library anyway
  ConnectionPool(clients::dns::Resolver& resolver,
                 const EndpointInfo& endpoint_info,
                 const AuthSettings& auth_settings,
                 const PoolSettings& pool_settings, bool use_secure_connection,
                 statistics::ConnectionStatistics& stats);

 private:
  friend class drivers::impl::ConnectionPoolBase<Connection, ConnectionPool>;

  ConnectionUniquePtr DoCreateConnection(engine::Deadline deadline);

  void AccountConnectionAcquired();
  void AccountConnectionReleased();
  void AccountConnectionCreated();
  void AccountConnectionDestroyed() noexcept;
  void AccountOverload();

  void RunMonitor();

  clients::dns::Resolver& resolver_;
  const EndpointInfo endpoint_info_;
  const AuthSettings auth_settings_;
  const PoolSettings pool_settings_;
  bool use_secure_connection_;
  statistics::ConnectionStatistics& stats_;

  utils::PeriodicTask monitor_;
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
