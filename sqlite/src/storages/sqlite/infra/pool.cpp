#include <memory>
#include <userver/storages/sqlite/infra/pool.hpp>

#include <chrono>

#include <userver/logging/log.hpp>
#include <userver/storages/sqlite/impl/connection_impl.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/utils/statistics/writer.hpp>
#include "userver/storages/sqlite/exceptions.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra {

namespace {

constexpr std::size_t kMaxSimultaneouslyConnectingClients{5};

constexpr std::chrono::milliseconds kConnectionSetupTimeout{2000};

// constexpr std::chrono::milliseconds kPoolSizeMonitorInterval{2000};

// constexpr std::chrono::milliseconds kPingerInterval{1000};
// constexpr std::chrono::milliseconds kPingTimeout{200};

}  // namespace

void DumpMetric(utils::statistics::Writer& writer,
                const PoolConnectionStatistics& stats) {
  writer["overload"] = stats.overload;
  writer["created"] = stats.created;
  writer["closed"] = stats.closed;

  writer["active"] = stats.created - stats.closed;
  writer["busy"] = stats.acquired - stats.released;
}

std::shared_ptr<Pool> Pool::Create(
    const settings::SQLiteSettings& settings,
    engine::TaskProcessor& blocking_task_processor) {
  return std::make_shared<Pool>(settings, blocking_task_processor);
}

Pool::~Pool() { Reset(); }

ConnectionPtr Pool::Acquire() {
  auto pool_and_connection = AcquireConnection({});

  return {std::move(pool_and_connection.pool_ptr),
          std::move(pool_and_connection.connection_ptr)};
}

void Pool::Release(ConnectionUniquePtr connection) {
  ReleaseConnection(std::move(connection));
}

Pool::Pool(const settings::SQLiteSettings& settings,
           engine::TaskProcessor& blocking_task_processor)
    : drivers::impl::ConnectionPoolBase<
          impl::ConnectionImpl, Pool>{settings.pool_settings.max_pool_size,
                                      kMaxSimultaneouslyConnectingClients},
      blocking_task_processor_{blocking_task_processor},
      settings_{settings} {
  try {
    Init(settings_.pool_settings.initial_pool_size, kConnectionSetupTimeout);
  } catch (const SQLiteException&) {
    Reset();
    throw;
  } catch (const std::exception&) {
  }
}

Pool::ConnectionUniquePtr Pool::DoCreateConnection(engine::Deadline) {
  try {
    auto connection_ptr = std::make_unique<impl::ConnectionImpl>(
        settings_, blocking_task_processor_);

    return connection_ptr;
  } catch (const std::exception&) {
    throw;
  }
}

void Pool::AccountConnectionAcquired() { ++stats_.acquired; }
void Pool::AccountConnectionReleased() { ++stats_.released; }
void Pool::AccountConnectionCreated() { ++stats_.created; }
void Pool::AccountConnectionDestroyed() noexcept { ++stats_.closed; }
void Pool::AccountOverload() { ++stats_.overload; }

void Pool::RunSizeMonitor() {
  if (AliveConnectionsCountApprox() <
      settings_.pool_settings.initial_pool_size) {
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

  // const auto pinger_connection_deleter = [this](ConnectionRawPtr connection)
  // {
  //   // To not touch given_away_semaphore accidentally
  //   DoRelease(ConnectionUniquePtr{connection});
  // };
  // const std::unique_ptr<impl::ConnectionImpl,
  //                       decltype(pinger_connection_deleter)>
  //     pinger_connection{connection_ptr.release(), pinger_connection_deleter};

  // try {
  //   pinger_connection->Ping(engine::Deadline::FromDuration(kPingTimeout));
  // } catch (const std::exception& ex) {
  //   LOG_WARNING() << "Failed to ping the server: " << ex.what();
  // }
}

}  // namespace storages::sqlite::infra

USERVER_NAMESPACE_END
