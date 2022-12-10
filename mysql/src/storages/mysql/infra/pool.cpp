#include <storages/mysql/infra/pool.hpp>

#include <chrono>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/wait_all_checked.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <storages/mysql/impl/mysql_connection.hpp>
#include <storages/mysql/infra/make_shared_enabler.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra {

namespace {

constexpr std::size_t kMaxSimultaneouslyConnectingClients{5};
constexpr std::size_t kMaxPoolSize{10};
constexpr std::size_t kMinPoolSize{1};

constexpr std::chrono::milliseconds kConnectionSetupTimeout{2000};

constexpr std::chrono::milliseconds kPoolSizeMonitorInterval{1000};

// TODO : think about these values
constexpr std::chrono::milliseconds kPingerInterval{330};
constexpr std::chrono::milliseconds kPingTimeout{200};

}  // namespace

std::shared_ptr<Pool> Pool::Create() {
  return std::make_shared<MakeSharedEnabler<Pool>>();
}

Pool::Pool()
    : given_away_semaphore_{kMaxPoolSize},
      connecting_semaphore_{kMaxSimultaneouslyConnectingClients},
      queue_{kMaxPoolSize},
      monitor_{*this} {
  try {
    std::vector<engine::TaskWithResult<void>> init_tasks;
    init_tasks.reserve(kMinPoolSize);

    for (std::size_t i = 0; i < kMinPoolSize; ++i) {
      init_tasks.emplace_back(engine::AsyncNoSpan([this] {
        PushConnection(engine::Deadline::FromDuration(kConnectionSetupTimeout));
      }));
    }

    engine::WaitAllChecked(init_tasks);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "Failed to properly setup connection pool: " << ex.what();
  }

  monitor_.Start();
}

Pool::~Pool() {
  monitor_.Stop();

  impl::MySQLConnection* connection_ptr{nullptr};
  while (queue_.pop(connection_ptr)) {
    Drop(connection_ptr);
  }
}

ConnectionPtr Pool::Acquire(engine::Deadline deadline) {
  return {shared_from_this(), Pop(deadline)};
}

void Pool::Release(std::unique_ptr<impl::MySQLConnection> connection) {
  DoRelease(std::move(connection));

  given_away_semaphore_.unlock_shared();
}

Pool::SmartConnectionPtr Pool::Pop(engine::Deadline deadline) {
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

  given_away_lock.Release();
  return connection_ptr;
}

Pool::SmartConnectionPtr Pool::TryPop() {
  impl::MySQLConnection* connection_ptr{nullptr};
  if (!queue_.pop(connection_ptr)) {
    return nullptr;
  }

  return SmartConnectionPtr{connection_ptr};
}

void Pool::DoRelease(SmartConnectionPtr connection) {
  auto* connection_ptr = connection.release();

  const auto broken = connection_ptr->IsBroken();
  if (broken) {
    monitor_.AccountFailure();
  }

  if (broken || !queue_.bounded_push(connection_ptr)) {
    Drop(connection_ptr);
  }
}

void Pool::PushConnection(engine::Deadline deadline) {
  auto* connection_ptr = CreateConnection(deadline).release();

  if (!queue_.bounded_push(connection_ptr)) {
    Drop(connection_ptr);
  }
}

Pool::SmartConnectionPtr Pool::CreateConnection(engine::Deadline /*deadline*/) {
  try {
    auto connection_ptr = std::make_unique<impl::MySQLConnection>();
    size_.fetch_add(1);
    monitor_.AccountSuccess();

    return connection_ptr;
  } catch (const std::exception&) {
    monitor_.AccountFailure();
    throw;
  }
}

void Pool::Drop(RawConnectionPtr connection) {
  std::default_delete<impl::MySQLConnection>{}(connection);

  size_.fetch_sub(1);
}

void Pool::RunSizeMonitor() {
  if (size_ < kMinPoolSize) {
    try {
      PushConnection(engine::Deadline::FromDuration(kConnectionSetupTimeout));
    } catch (const std::exception& ex) {
      LOG_WARNING() << "Failed to add a connection into pool: " << ex;
    }
  }
}

void Pool::RunPinger() {
  auto connection_ptr = TryPop();
  if (!connection_ptr) {
    return;
  }

  const auto pinger_connection_deleter = [this](RawConnectionPtr connection) {
    // To not touch given_away_semaphore accidentally
    DoRelease(SmartConnectionPtr{connection});
  };
  std::unique_ptr<impl::MySQLConnection, decltype(pinger_connection_deleter)>
      pinger_connection{connection_ptr.release(), pinger_connection_deleter};

  try {
    pinger_connection->Ping(engine::Deadline::FromDuration(kPingTimeout));
    monitor_.AccountSuccess();
  } catch (const std::exception& ex) {
    LOG_WARNING() << "Failed to ping the server: " << ex.what();
  }
}

Pool::PoolMonitor::PoolMonitor(Pool& pool) : pool_{pool} {}

Pool::PoolMonitor::~PoolMonitor() { Stop(); }

void Pool::PoolMonitor::Start() {
  size_monitor_.Start("mysql_connection_pool_monitor",
                      {{kPoolSizeMonitorInterval}},
                      [this] { pool_.RunSizeMonitor(); });
  pinger_.Start("mysql_connection_pool_pinger", {{kPingerInterval}},
                [this] { pool_.RunPinger(); });
}

void Pool::PoolMonitor::Stop() {
  pinger_.Stop();
  size_monitor_.Stop();
}

void Pool::PoolMonitor::AccountSuccess() noexcept {
  last_success_.store(Clock::now());
}

void Pool::PoolMonitor::AccountFailure() noexcept {
  last_failure_.store(Clock::now());
}

bool Pool::PoolMonitor::IsAvailable() noexcept {
  const auto now = Clock::now();

  const auto last_success = last_success_.load();
  const auto last_failure = last_failure_.load();

  if (last_success > last_failure) {
    return true;
  } else {
    return last_failure + kUnavailabilityThreshold > now;
  }
}

}  // namespace storages::mysql::infra

USERVER_NAMESPACE_END
