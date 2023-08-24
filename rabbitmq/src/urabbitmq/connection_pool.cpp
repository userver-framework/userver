#include "connection_pool.hpp"

#include <urabbitmq/connection.hpp>
#include <urabbitmq/connection_ptr.hpp>
#include <urabbitmq/make_shared_enabler.hpp>
#include <urabbitmq/statistics/connection_statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

namespace {

constexpr std::chrono::milliseconds kConnectionSetupTimeout{2000};
constexpr std::chrono::milliseconds kPoolMonitorInterval{1000};

constexpr size_t kMaxSimultaneouslyConnectingClients{5};

}  // namespace

std::shared_ptr<ConnectionPool> ConnectionPool::Create(
    clients::dns::Resolver& resolver, const EndpointInfo& endpoint_info,
    const AuthSettings& auth_settings, const PoolSettings& pool_settings,
    bool use_secure_connection, statistics::ConnectionStatistics& stats) {
  return std::make_shared<ConnectionPool>(resolver, endpoint_info,
                                          auth_settings, pool_settings,
                                          use_secure_connection, stats);
}

ConnectionPool::ConnectionPool(clients::dns::Resolver& resolver,
                               const EndpointInfo& endpoint_info,
                               const AuthSettings& auth_settings,
                               const PoolSettings& pool_settings,
                               bool use_secure_connection,
                               statistics::ConnectionStatistics& stats)
    : drivers::impl::ConnectionPoolBase<
          Connection, ConnectionPool>{pool_settings.max_pool_size,
                                      kMaxSimultaneouslyConnectingClients},
      resolver_{resolver},
      endpoint_info_{endpoint_info},
      auth_settings_{auth_settings},
      pool_settings_{pool_settings},
      use_secure_connection_{use_secure_connection},
      stats_{stats} {
  try {
    Init(pool_settings_.min_pool_size, kConnectionSetupTimeout);
  } catch (const impl::ConnectionSetupError& ex) {
    // this error is thrown when connection creation fails explicitly,
    // which is caused by either invalid auth or hitting some limits
    // in RabbitMQ (per-vhost/per-user/global/etc. connections limit).
    // In both cases it seems severe enough to fail completely.

    LOG_ERROR() << "Critical failure encountered in connection pool setup: "
                << ex.what();
    Reset();
    throw;
  } catch (const std::exception&) {
    // Already logged in base class
  }

  monitor_.Start("connection_pool_monitor", {{kPoolMonitorInterval}},
                 [this] { RunMonitor(); });
}

ConnectionPool::~ConnectionPool() {
  monitor_.Stop();

  Reset();
}

ConnectionPtr ConnectionPool::Acquire(engine::Deadline deadline) {
  auto pool_and_connection = AcquireConnection(deadline);

  return {std::move(pool_and_connection.pool_ptr),
          std::move(pool_and_connection.connection_ptr)};
}

void ConnectionPool::Release(std::unique_ptr<Connection> connection) {
  UASSERT(connection);

  ReleaseConnection(std::move(connection));
}

void ConnectionPool::NotifyConnectionAdopted() {
  NotifyConnectionWontBeReleased();
}

ConnectionPool::ConnectionUniquePtr ConnectionPool::DoCreateConnection(
    engine::Deadline deadline) {
  return std::make_unique<Connection>(resolver_, endpoint_info_, auth_settings_,
                                      pool_settings_.max_in_flight_requests,
                                      use_secure_connection_, stats_, deadline);
}

void ConnectionPool::RunMonitor() {
  if (AliveConnectionsCountApprox() < pool_settings_.min_pool_size) {
    try {
      PushConnection(engine::Deadline::FromDuration(kConnectionSetupTimeout));
    } catch (const std::exception& ex) {
      LOG_WARNING() << "Failed to add connection into pool: " << ex;
    }
  }
}

void ConnectionPool::AccountConnectionAcquired() {}

void ConnectionPool::AccountConnectionReleased() {}

void ConnectionPool::AccountConnectionCreated() {
  stats_.AccountConnectionCreated();
}

void ConnectionPool::AccountConnectionDestroyed() noexcept {
  stats_.AccountConnectionClosed();
}

void ConnectionPool::AccountOverload() {}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
