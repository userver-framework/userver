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

constexpr std::chrono::milliseconds kConnectionSetupTimeout{2000};
constexpr std::chrono::milliseconds kPoolMonitorInterval{1000};

std::shared_ptr<ConnectionPool> ConnectionPool::Create(
    clients::dns::Resolver& resolver, const EndpointInfo& endpoint_info,
    const AuthSettings& auth_settings, const PoolSettings& pool_settings,
    statistics::ConnectionStatistics& stats) {
  return std::make_shared<MakeSharedEnabler<ConnectionPool>>(
      resolver, endpoint_info, auth_settings, pool_settings, stats);
}

ConnectionPool::ConnectionPool(clients::dns::Resolver& resolver,
                               const EndpointInfo& endpoint_info,
                               const AuthSettings& auth_settings,
                               const PoolSettings& pool_settings,
                               statistics::ConnectionStatistics& stats)
    : resolver_{resolver},
      endpoint_info_{endpoint_info},
      auth_settings_{auth_settings},
      pool_settings_{pool_settings},
      stats_{stats},
      queue_{pool_settings_.max_pool_size} {
  std::vector<engine::TaskWithResult<void>> init_tasks;
  init_tasks.reserve(pool_settings_.min_pool_size);

  for (size_t i = 0; i < pool_settings_.min_pool_size; ++i) {
    init_tasks.emplace_back(engine::AsyncNoSpan([this] { PushConnection(); }));
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

ConnectionPtr ConnectionPool::Acquire() { return {shared_from_this(), Pop()}; }

void ConnectionPool::Release(std::unique_ptr<Connection> conn) {
  UASSERT(conn);

  auto* ptr = conn.release();
  if (ptr->IsBroken() || !queue_.bounded_push(ptr)) {
    Drop(ptr);
  }
}

void ConnectionPool::NotifyConnectionAdopted() { size_.fetch_sub(1); }

std::unique_ptr<Connection> ConnectionPool::Pop() {
  auto connection_ptr = TryPop();
  if (!connection_ptr) {
    throw std::runtime_error{"TODO"};
  }

  return connection_ptr;
}

std::unique_ptr<Connection> ConnectionPool::TryPop() {
  Connection* conn{nullptr};
  if (!queue_.pop(conn)) {
    return nullptr;
  }

  return std::unique_ptr<Connection>(conn);
}

void ConnectionPool::PushConnection() {
  auto* ptr = Create().release();
  if (!queue_.bounded_push(ptr)) {
    Drop(ptr);
  }

  size_.fetch_add(1);
}

std::unique_ptr<Connection> ConnectionPool::Create() {
  auto conn = std::make_unique<Connection>(
      resolver_, endpoint_info_, auth_settings_, pool_settings_.secure, stats_,
      engine::Deadline::FromDuration(kConnectionSetupTimeout));

  stats_.AccountConnectionCreated();
  return conn;
}

void ConnectionPool::Drop(Connection* conn) noexcept {
  std::default_delete<Connection>{}(conn);

  stats_.AccountConnectionClosed();
  size_.fetch_sub(1);
}

void ConnectionPool::RunMonitor() {
  if (size_ < pool_settings_.min_pool_size) {
    try {
      PushConnection();
    } catch (const std::exception& ex) {
      LOG_WARNING() << "Failed to add connection into pool: " << ex;
    }
  }
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END