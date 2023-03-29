#include "pool_impl.hpp"

#include <string>

#include <storages/clickhouse/impl/connection.hpp>
#include <storages/clickhouse/impl/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

namespace {

constexpr size_t kMaxSimultaneouslyConnectingClients{5};
constexpr std::chrono::milliseconds kConnectTimeout{2000};

constexpr std::chrono::seconds kMaintenanceInterval{2};
constexpr std::chrono::seconds PoolUnavailableThreshold{60};
static_assert(PoolUnavailableThreshold > kMaintenanceInterval);

const std::string kMaintenanceTaskName = "clickhouse_maintain";

}  // namespace

bool PoolAvailabilityMonitor::IsAvailable() const {
  const auto last_successful_communication =
      last_successful_communication_.load();
  if (last_successful_communication == TimePoint{}) {
    return last_unsuccessful_communication_.load() == TimePoint{};
  }

  const auto now = Clock::now();
  return std::chrono::duration_cast<std::chrono::seconds>(
             now - last_successful_communication) < PoolUnavailableThreshold;
}

void PoolAvailabilityMonitor::AccountSuccess() noexcept {
  last_successful_communication_ = Clock::now();
}

void PoolAvailabilityMonitor::AccountFailure() noexcept {
  last_unsuccessful_communication_ = Clock::now();
}

struct PoolImpl::MaintenanceConnectionDeleter final {
  void operator()(Connection* connection_ptr) noexcept {
    const auto is_broken = connection_ptr->IsBroken();
    if (!is_broken) {
      pool.availability_monitor_.AccountSuccess();
    }

    pool.DoRelease(ConnectionUniquePtr{connection_ptr});
  }

  PoolImpl& pool;
};

PoolImpl::PoolImpl(clients::dns::Resolver& resolver, PoolSettings&& settings)
    : drivers::impl::ConnectionPoolBase<
          Connection, PoolImpl>{settings.max_pool_size,
                                kMaxSimultaneouslyConnectingClients},
      resolver_{resolver},
      pool_settings_{std::move(settings)} {
  try {
    Init(pool_settings_.initial_pool_size, kConnectTimeout);
  } catch (const std::exception&) {
    // This is already logged in base class, and it's also fine:
    // host might be under maintenance currently and will become alive at some
    // point.
    // TODO : rethrow on auth errors, these are fatal
  }
}

PoolImpl::~PoolImpl() {
  StopMaintenance();

  Reset();
}

bool PoolImpl::IsAvailable() const {
  return availability_monitor_.IsAvailable();
}

ConnectionPtr PoolImpl::Acquire() {
  auto pool_and_connection = AcquireConnection(
      engine::Deadline::FromDuration(pool_settings_.queue_timeout));

  return {std::move(pool_and_connection.pool_ptr),
          pool_and_connection.connection_ptr.release()};
}

void PoolImpl::Release(Connection* connection_ptr) {
  UASSERT(connection_ptr);

  const auto is_broken = connection_ptr->IsBroken();
  if (!is_broken) {
    availability_monitor_.AccountSuccess();
  }

  ReleaseConnection(ConnectionUniquePtr{connection_ptr});
}

stats::PoolStatistics& PoolImpl::GetStatistics() noexcept {
  return statistics_;
}

const std::string& PoolImpl::GetHostName() const {
  return pool_settings_.endpoint_settings.host;
}

stats::StatementTimer PoolImpl::GetInsertTimer() {
  return stats::StatementTimer{statistics_.inserts};
}

stats::StatementTimer PoolImpl::GetExecuteTimer() {
  return stats::StatementTimer{statistics_.queries};
}

void PoolImpl::AccountConnectionAcquired() {
  ++GetStatistics().connections.busy;
}

void PoolImpl::AccountConnectionReleased() {
  --GetStatistics().connections.busy;
}

void PoolImpl::AccountConnectionCreated() {
  auto& stats = GetStatistics().connections;
  ++stats.created;
  ++stats.active;
}

void PoolImpl::AccountConnectionDestroyed() noexcept {
  auto& stats = GetStatistics().connections;
  ++stats.closed;
  --stats.active;
}

void PoolImpl::AccountOverload() { ++GetStatistics().connections.overload; }

PoolImpl::ConnectionUniquePtr PoolImpl::DoCreateConnection(engine::Deadline) {
  try {
    return std::make_unique<Connection>(
        resolver_, pool_settings_.endpoint_settings,
        pool_settings_.auth_settings, pool_settings_.connection_settings);
  } catch (const std::exception&) {
    availability_monitor_.AccountFailure();
    throw;
  }
}

void PoolImpl::StartMaintenance() {
  using PeriodicTask = USERVER_NAMESPACE::utils::PeriodicTask;

  maintenance_task_.Start(
      kMaintenanceTaskName,
      {kMaintenanceInterval,
       {PeriodicTask::Flags::kStrong, PeriodicTask::Flags::kCritical}},
      [this] { MaintainConnections(); });
}

void PoolImpl::StopMaintenance() { maintenance_task_.Stop(); }

void PoolImpl::MaintainConnections() {
  const auto failsafe_push_connection = [this] {
    try {
      PushConnection(engine::Deadline::FromDuration(kConnectTimeout));
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Failed to create connection: " << ex;
    }
  };

  ConnectionUniquePtr connection_ptr = TryPop();
  if (!connection_ptr) {
    if (AliveConnectionsCountApprox() < pool_settings_.initial_pool_size) {
      failsafe_push_connection();
    }

    return;
  }

  {
    using Deleter = MaintenanceConnectionDeleter;
    auto conn = std::unique_ptr<Connection, Deleter>{connection_ptr.release(),
                                                     Deleter{*this}};
    try {
      conn->Ping();
    } catch (const std::exception& ex) {
      LOG_LIMITED_WARNING() << "Exception while pinging connection to '"
                            << GetHostName() << "': " << ex;
    }
  }

  if (AliveConnectionsCountApprox() < pool_settings_.initial_pool_size) {
    failsafe_push_connection();
  }
}

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
