#include "connection_pool.hpp"

#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/wait_all_checked.hpp>

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
    bool use_tls, statistics::ConnectionStatistics& stats) {
  // FP?: pointer magic in boost.lockfree
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
  return std::make_shared<MakeSharedEnabler<ConnectionPool>>(
      resolver, endpoint_info, auth_settings, pool_settings, use_tls, stats);
}

ConnectionPool::ConnectionPool(clients::dns::Resolver& resolver,
                               const EndpointInfo& endpoint_info,
                               const AuthSettings& auth_settings,
                               const PoolSettings& pool_settings,
                               bool use_secure_connection,
                               statistics::ConnectionStatistics& stats)
    : resolver_{resolver},
      endpoint_info_{endpoint_info},
      auth_settings_{auth_settings},
      pool_settings_{pool_settings},
      use_secure_connection_{use_secure_connection},
      stats_{stats},
      given_away_semaphore_{pool_settings_.max_pool_size},
      connecting_semaphore_{kMaxSimultaneouslyConnectingClients},
      // FP?: pointer magic in boost.lockfree
      // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
      queue_{pool_settings_.max_pool_size} {
  std::vector<engine::TaskWithResult<void>> init_tasks;
  init_tasks.reserve(pool_settings_.min_pool_size);

  for (size_t i = 0; i < pool_settings_.min_pool_size; ++i) {
    init_tasks.emplace_back(engine::AsyncNoSpan([this] {
      PushConnection(engine::Deadline::FromDuration(kConnectionSetupTimeout));
    }));
  }
  try {
    engine::WaitAllChecked(init_tasks);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "Failed to properly setup connection pool: " << ex.what();
  }

  monitor_.Start("connection_pool_monitor", {{kPoolMonitorInterval}},
                 [this] { RunMonitor(); });
}

ConnectionPool::~ConnectionPool() {
  monitor_.Stop();

  while (true) {
    Connection* conn{nullptr};
    if (!queue_.pop(conn)) {
      break;
    }

    Drop(conn);
  }
}

ConnectionPtr ConnectionPool::Acquire(engine::Deadline deadline) {
  return {shared_from_this(), Pop(deadline)};
}

void ConnectionPool::Release(std::unique_ptr<Connection> connection) {
  UASSERT(connection);

  auto* ptr = connection.release();
  if (ptr->IsBroken() || !queue_.bounded_push(ptr)) {
    Drop(ptr);
  }

  given_away_semaphore_.unlock_shared();
}

void ConnectionPool::NotifyConnectionAdopted() {
  given_away_semaphore_.unlock_shared();
  size_.fetch_sub(1);
}

std::unique_ptr<Connection> ConnectionPool::Pop(engine::Deadline deadline) {
  engine::SemaphoreLock given_away_lock{given_away_semaphore_, deadline};
  if (!given_away_lock.OwnsLock()) {
    throw std::runtime_error{"Connection pool acquisition wait limit exceeded"};
  }

  auto connection_ptr = TryPop();
  if (!connection_ptr) {
    engine::SemaphoreLock connecting_lock{connecting_semaphore_, deadline};

    connection_ptr = TryPop();
    if (!connection_ptr) {
      if (!connecting_lock.OwnsLock()) {
        throw std::runtime_error{
            "Connection pool acquisition wait limit exceeded"};
      }
      connection_ptr = CreateConnection(deadline);
    }
  }

  UASSERT(connection_ptr);
  // sad part: connection can actually be broken at this point, but handling
  // it here makes little sense: if it has outstanding operations it could
  // become broken right after we check. This behavior is documented in
  // client_settings, so we should be good to go.

  given_away_lock.Release();
  return connection_ptr;
}

std::unique_ptr<Connection> ConnectionPool::TryPop() {
  Connection* conn{nullptr};
  if (!queue_.pop(conn)) {
    return nullptr;
  }

  return std::unique_ptr<Connection>(conn);
}

void ConnectionPool::PushConnection(engine::Deadline deadline) {
  auto* ptr = CreateConnection(deadline).release();
  if (!queue_.bounded_push(ptr)) {
    Drop(ptr);
  }
}

std::unique_ptr<Connection> ConnectionPool::CreateConnection(
    engine::Deadline deadline) {
  auto conn =
      std::make_unique<Connection>(resolver_, endpoint_info_, auth_settings_,
                                   pool_settings_.max_in_flight_requests,
                                   use_secure_connection_, stats_, deadline);

  stats_.AccountConnectionCreated();
  size_.fetch_add(1);
  return conn;
}

void ConnectionPool::Drop(Connection* connection) noexcept {
  std::default_delete<Connection>{}(connection);

  stats_.AccountConnectionClosed();
  size_.fetch_sub(1);
}

void ConnectionPool::RunMonitor() {
  if (size_ < pool_settings_.min_pool_size) {
    try {
      PushConnection(engine::Deadline::FromDuration(kConnectionSetupTimeout));
    } catch (const std::exception& ex) {
      LOG_WARNING() << "Failed to add connection into pool: " << ex;
    }
  }
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
