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

constexpr size_t kInitialPoolSize = 10;

std::shared_ptr<ConnectionPool> ConnectionPool::Create(
    clients::dns::Resolver& resolver, const EndpointInfo& endpoint_info,
    const AuthSettings& auth_settings,
    statistics::ConnectionStatistics& stats) {
  return std::make_shared<MakeSharedEnabler<ConnectionPool>>(
      resolver, endpoint_info, auth_settings, stats);
}

ConnectionPool::ConnectionPool(clients::dns::Resolver& resolver,
                               const EndpointInfo& endpoint_info,
                               const AuthSettings& auth_settings,
                               statistics::ConnectionStatistics& stats)
    : resolver_{resolver},
      endpoint_info_{endpoint_info},
      auth_settings_{auth_settings},
      stats_{stats},
      queue_{kInitialPoolSize} {
  std::vector<engine::TaskWithResult<void>> init_tasks;
  init_tasks.reserve(kInitialPoolSize);

  for (size_t i = 0; i < kInitialPoolSize; ++i) {
    init_tasks.emplace_back(engine::AsyncNoSpan([this] { PushConnection(); }));
  }
  try {
    engine::WaitAllChecked(init_tasks);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "Failed to properly setup connection pool: " << ex.what();
  }
}

ConnectionPool::~ConnectionPool() {
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
  conn->ResetCallbacks();

  auto ptr = conn.release();
  if (ptr->IsBroken() || !queue_.bounded_push(ptr)) {
    Drop(ptr);
  }
}

std::unique_ptr<Connection> ConnectionPool::Pop() {
  auto connection_ptr = TryPop();
  if (!connection_ptr) {
    throw std::runtime_error{"TODO"};
  }

  return connection_ptr;
}

std::unique_ptr<Connection> ConnectionPool::TryPop() {
  Connection* conn{nullptr};
  queue_.pop(conn);

  return std::unique_ptr<Connection>(conn);
}

void ConnectionPool::PushConnection() {
  auto* ptr = Create().release();
  if (!queue_.bounded_push(ptr)) {
    Drop(ptr);
  }
}

std::unique_ptr<Connection> ConnectionPool::Create() {
  return std::make_unique<Connection>(resolver_, endpoint_info_, auth_settings_,
                                      stats_);
}

void ConnectionPool::Drop(Connection* conn) noexcept {
  std::default_delete<Connection>{}(conn);
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END