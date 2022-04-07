#include "pool_impl.hpp"

#include <vector>

#include <userver/utils/scope_guard.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/get_all.hpp>

#include <storages/clickhouse/impl/connection.hpp>
#include <storages/clickhouse/impl/connection_mode.hpp>
#include <storages/clickhouse/impl/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

namespace {

constexpr size_t kMaxSimultaneouslyConnectingClients{5};

struct ConnectionDeleter final {
  void operator()(Connection* conn) { delete conn; }
};

ConnectionMode GetConnectionMode(bool use_secure_connection) {
  return use_secure_connection ? ConnectionMode::kSecure
                               : ConnectionMode::kNonSecure;
}

}  // namespace

PoolImpl::PoolImpl(clients::dns::Resolver* resolver, PoolSettings&& settings)
    : pool_settings_{std::move(settings)},
      given_away_semaphore_{pool_settings_.max_pool_size},
      connecting_semaphore_{kMaxSimultaneouslyConnectingClients},
      queue_{pool_settings_.max_pool_size},
      resolver_{resolver} {
  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.reserve(pool_settings_.initial_pool_size);
  for (size_t i = 0; i < pool_settings_.initial_pool_size; ++i) {
    tasks.emplace_back(engine::AsyncNoSpan([this] {
      try {
        queue_.bounded_push(Create());
      } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to create connection: " << e;
      }
    }));
  }
  engine::GetAll(tasks);
}

PoolImpl::~PoolImpl() {
  Connection* conn = nullptr;
  // boost.lockfree pointer magic (FP?)
  // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
  while (queue_.pop(conn)) Drop(conn);
}

ConnectionPtr PoolImpl::Acquire() {
  return ConnectionPtr(shared_from_this(), Pop());
}

void PoolImpl::Release(Connection* conn) {
  UASSERT(conn);

  if (conn->IsBroken() || !queue_.bounded_push(conn)) Drop(conn);
  given_away_semaphore_.unlock_shared();
}

stats::PoolStatistics& PoolImpl::GetStatistics() { return statistics_; }

const std::string& PoolImpl::GetHostName() const {
  return pool_settings_.endpoint_settings.host;
}

Connection* PoolImpl::Create() {
  auto conn = std::make_unique<Connection>(
      resolver_, pool_settings_.endpoint_settings, pool_settings_.auth_settings,
      GetConnectionMode(pool_settings_.use_secure_connection));
  ++GetStatistics().created;

  return conn.release();
}

void PoolImpl::Drop(Connection* conn) {
  ConnectionDeleter{}(conn);
  ++GetStatistics().closed;
}

Connection* PoolImpl::Pop() {
  const auto deadline =
      engine::Deadline::FromDuration(pool_settings_.queue_timeout);

  engine::SemaphoreLock given_away_lock{given_away_semaphore_, deadline};
  if (!given_away_lock) {
    ++GetStatistics().overload;
    throw std::runtime_error{"queue wait limit exceeded"};
  }

  auto* conn = TryPop();
  if (!conn) {
    engine::SemaphoreLock connecting_lock{connecting_semaphore_, deadline};

    conn = TryPop();
    if (!conn) {
      if (!connecting_lock) {
        ++GetStatistics().overload;
        throw std::runtime_error{"connection queue wait limit exceeded"};
      }
      conn = Create();
    }
  }

  UASSERT(conn);
  UASSERT(!conn->IsBroken());
  given_away_lock.Release();
  return conn;
}

Connection* PoolImpl::TryPop() {
  Connection* conn = nullptr;
  if (queue_.pop(conn)) return conn;

  return nullptr;
}

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
