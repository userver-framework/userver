#include <userver/storages/sqlite/infra/pool.hpp>

#include <chrono>
#include <memory>

#include <userver/logging/log.hpp>
#include <userver/utils/statistics/writer.hpp>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/impl/connection.hpp>
#include <userver/storages/sqlite/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra {

namespace {

constexpr std::size_t kMaxSimultaneouslyConnectingClients{5};

constexpr std::chrono::milliseconds kConnectionSetupTimeout{2000};

}  // namespace

std::shared_ptr<Pool>
Pool::Create(const settings::SQLiteSettings& settings, engine::TaskProcessor& blocking_task_processor) {
    return std::make_shared<Pool>(settings, blocking_task_processor);
}

Pool::~Pool() { Reset(); }

ConnectionPtr Pool::Acquire() {
    auto pool_and_connection = AcquireConnection({});

    return {std::move(pool_and_connection.pool_ptr), std::move(pool_and_connection.connection_ptr)};
}

void Pool::Release(ConnectionUniquePtr connection) { ReleaseConnection(std::move(connection)); }

Pool::Pool(const settings::SQLiteSettings& settings, engine::TaskProcessor& blocking_task_processor)
    : drivers::impl::ConnectionPoolBase<
          impl::Connection,
          Pool>{settings.pool_settings.max_pool_size, kMaxSimultaneouslyConnectingClients},
      blocking_task_processor_{blocking_task_processor},
      settings_{settings} {
    try {
        Init(settings_.pool_settings.initial_pool_size, kConnectionSetupTimeout);
    } catch (const SQLiteException&) {
        Reset();
        throw;
    } catch (const std::exception& err) {
        LOG_ERROR() << "Error in connection pool: " << err.what();
    }
}

statistics::PoolStatistics& Pool::GetStatistics() { return stats_; }

Pool::ConnectionUniquePtr Pool::DoCreateConnection(engine::Deadline) {
    try {
        auto connection_ptr = std::make_unique<impl::Connection>(settings_, blocking_task_processor_, GetStatistics());

        return connection_ptr;
    } catch (const std::exception&) {
        throw;
    }
}

void Pool::AccountConnectionAcquired() { ++stats_.connections.acquired; }
void Pool::AccountConnectionReleased() { ++stats_.connections.released; }
void Pool::AccountConnectionCreated() { ++stats_.connections.created; }
void Pool::AccountConnectionDestroyed() noexcept { ++stats_.connections.closed; }
void Pool::AccountOverload() { ++stats_.connections.overload; }

}  // namespace storages::sqlite::infra

USERVER_NAMESPACE_END
