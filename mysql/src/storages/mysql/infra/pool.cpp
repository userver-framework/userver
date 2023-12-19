#include <storages/mysql/infra/pool.hpp>

#include <chrono>

#include <userver/logging/log.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <storages/mysql/impl/connection.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra {

namespace {

constexpr std::size_t kMaxSimultaneouslyConnectingClients{5};

constexpr std::chrono::milliseconds kConnectionSetupTimeout{2000};

constexpr std::chrono::milliseconds kPoolSizeMonitorInterval{2000};

constexpr std::chrono::milliseconds kPingerInterval{1000};
constexpr std::chrono::milliseconds kPingTimeout{200};

}  // namespace

std::shared_ptr<Pool> Pool::Create(
    clients::dns::Resolver& resolver,
    const settings::PoolSettings& pool_settings) {
  return std::make_shared<Pool>(resolver, pool_settings);
}

Pool::~Pool() {
  monitor_.Stop();

  Reset();
}

ConnectionPtr Pool::Acquire(engine::Deadline deadline) {
  auto pool_and_connection = AcquireConnection(deadline);

  return {std::move(pool_and_connection.pool_ptr),
          std::move(pool_and_connection.connection_ptr)};
}

void Pool::Release(ConnectionUniquePtr connection) {
  ReleaseConnection(std::move(connection));
}

void Pool::WriteStatistics(utils::statistics::Writer& writer) const {
  writer.ValueWithLabels(stats_,
                         {{"mysql_instance", settings_.endpoint_info.host}});
}

Pool::Pool(clients::dns::Resolver& resolver,
           const settings::PoolSettings& pool_settings)
    : drivers::impl::ConnectionPoolBase<
          impl::Connection, Pool>{pool_settings.max_pool_size,
                                  kMaxSimultaneouslyConnectingClients},
      resolver_{resolver},
      settings_{pool_settings},
      monitor_{*this} {
  try {
    Init(settings_.initial_pool_size, kConnectionSetupTimeout);
  } catch (const std::exception&) {
  }

  monitor_.Start();
}

Pool::ConnectionUniquePtr Pool::DoCreateConnection(engine::Deadline deadline) {
  try {
    auto connection_ptr = std::make_unique<impl::Connection>(
        resolver_, settings_.endpoint_info, settings_.auth_settings,
        settings_.connection_settings, deadline);
    monitor_.AccountSuccess();

    return connection_ptr;
  } catch (const std::exception&) {
    monitor_.AccountFailure();
    throw;
  }
}

void Pool::AccountConnectionAcquired() { ++stats_.acquired; }
void Pool::AccountConnectionReleased() { ++stats_.released; }
void Pool::AccountConnectionCreated() { ++stats_.created; }
void Pool::AccountConnectionDestroyed() noexcept { ++stats_.closed; }
void Pool::AccountOverload() { ++stats_.overload; }

void Pool::RunSizeMonitor() {
  if (AliveConnectionsCountApprox() < settings_.initial_pool_size) {
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

  const auto pinger_connection_deleter = [this](ConnectionRawPtr connection) {
    // To not touch given_away_semaphore accidentally
    DoRelease(ConnectionUniquePtr{connection});
  };
  const std::unique_ptr<impl::Connection, decltype(pinger_connection_deleter)>
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
