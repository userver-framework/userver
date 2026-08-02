#include <storages/odbc/detail/pool.hpp>

#include <storages/odbc/detail/deadline.hpp>
#include <userver/drivers/impl/connection_pool_base.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/odbc/exception.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

namespace {

auto constexpr kConnectionSetupTimeout = std::chrono::milliseconds{2000};
auto constexpr kMaxSimultaneouslyConnectingClients = std::size_t{5};
auto constexpr kPoolSizeMonitorInterval = std::chrono::milliseconds{2000};

}  // namespace

Pool::Pool(
    const std::string& dsn,
    std::size_t min_pool_size,
    std::size_t max_pool_size,
    engine::TaskProcessor& blocking_task_processor,
    const settings::StatementMetricsSettings& statement_metrics_settings,
    const settings::PreparedStatementCacheSettings& prepared_statement_cache_settings
)
    : ConnectionPoolBase<Connection, Pool>(max_pool_size, kMaxSimultaneouslyConnectingClients),
      dsns_({dsn}),
      min_pool_size_(min_pool_size),
      max_pool_size_(max_pool_size),
      blocking_task_processor_{blocking_task_processor},
      statement_stats_{statement_metrics_settings},
      prepared_statement_cache_state_{std::make_shared<PreparedStatementCacheState>(prepared_statement_cache_settings)}
{
    stats_.connection.maximum = max_pool_size;
    try {
        Init(min_pool_size_, kConnectionSetupTimeout);
    } catch (const std::exception& ex) {
        // Temporary startup outages are recovered by the size monitor. Any
        // connections that did initialize successfully remain available.
        LOG_WARNING() << "ODBC pool initialized below min_pool_size: " << ex;
    } catch (...) {
        LOG_WARNING() << "ODBC pool initialized below min_pool_size due to an unknown error";
    }
    size_monitor_.Start("odbc_connection_pool_monitor", {{kPoolSizeMonitorInterval}}, [this] { RunSizeMonitor(); });
}

Pool::Pool(
    std::vector<std::string> dsns,
    std::size_t min_pool_size,
    std::size_t max_pool_size,
    engine::TaskProcessor& blocking_task_processor,
    const settings::StatementMetricsSettings& statement_metrics_settings,
    const settings::PreparedStatementCacheSettings& prepared_statement_cache_settings
)
    : ConnectionPoolBase<Connection, Pool>(max_pool_size, kMaxSimultaneouslyConnectingClients),
      dsns_(std::move(dsns)),
      min_pool_size_(min_pool_size),
      max_pool_size_(max_pool_size),
      blocking_task_processor_{blocking_task_processor},
      statement_stats_{statement_metrics_settings},
      prepared_statement_cache_state_{std::make_shared<PreparedStatementCacheState>(prepared_statement_cache_settings)}
{
    stats_.connection.maximum = max_pool_size;
    try {
        Init(min_pool_size_, kConnectionSetupTimeout);
    } catch (const std::exception& ex) {
        LOG_WARNING() << "ODBC pool initialized below min_pool_size: " << ex;
    } catch (...) {
        LOG_WARNING() << "ODBC pool initialized below min_pool_size due to an unknown error";
    }
    size_monitor_.Start("odbc_connection_pool_monitor", {{kPoolSizeMonitorInterval}}, [this] { RunSizeMonitor(); });
}

Pool::~Pool() {
    size_monitor_.Stop();
    Reset();
}

StatementStatisticsSnapshot Pool::GetStatementStatistics() const { return statement_stats_.GetStatementsStats(); }

void Pool::SetStatementMetricsSettings(const settings::StatementMetricsSettings& settings) {
    statement_stats_.SetSettings(settings);
}

const PreparedStatementCacheStatistics& Pool::GetPreparedStatementCacheStatistics() const noexcept {
    return prepared_statement_cache_state_->GetStatistics();
}

void Pool::SetPreparedStatementCacheSettings(const settings::PreparedStatementCacheSettings& settings) {
    prepared_statement_cache_state_->SetSettings(settings);
}

ConnectionPtr Pool::Acquire(engine::Deadline deadline) {
    const auto start = utils::datetime::SteadyCoarseClock::now();
    ++stats_.connection.waiting;
    const utils::FastScopeGuard waiting_guard([this]() noexcept { --stats_.connection.waiting; });

    decltype(AcquireConnection(deadline)) conn_wrapper;
    try {
        conn_wrapper = AcquireConnection(deadline);
    } catch (const drivers::impl::PoolWaitLimitExceededError&) {
        CheckDeadlineNotExpired(deadline);
        if (engine::current_task::ShouldCancel()) {
            throw OperationInterrupted("Cancelled while waiting for an ODBC connection");
        }
        throw PoolError("ODBC connection pool wait limit exceeded");
    }

    ++stats_.connection.used;

    const auto elapsed = std::chrono::duration_cast<
        std::chrono::milliseconds>(utils::datetime::SteadyCoarseClock::now() - start);
    stats_.acquire_percentile.Account(elapsed.count());

    return {std::move(conn_wrapper.pool_ptr), std::move(conn_wrapper.connection_ptr)};
}

void Pool::Release(ConnectionUniquePtr connection) {
    --stats_.connection.used;
    ReleaseConnection(std::move(connection));
}

Pool::ConnectionUniquePtr Pool::DoCreateConnection(engine::Deadline deadline) {
    const auto start = utils::datetime::SteadyCoarseClock::now();
    try {
        CheckDeadlineNotExpired(deadline);
        const auto idx = dsn_index_.fetch_add(1);
        auto conn = std::make_unique<
            Connection>(dsns_[idx % dsns_.size()], blocking_task_processor_, deadline, prepared_statement_cache_state_);
        ++stats_.connection.open_total;

        const auto elapsed = std::chrono::duration_cast<
            std::chrono::milliseconds>(utils::datetime::SteadyCoarseClock::now() - start);
        stats_.connection_percentile.Account(elapsed.count());

        return conn;
    } catch (const OperationInterrupted& ex) {
        ++stats_.connection.error_timeout;
        LOG_ERROR() << "Timed out while creating ODBC connection: " << ex;
        throw;
    } catch (const std::exception& ex) {
        ++stats_.connection.error_total;
        LOG_ERROR() << "Failed to create ODBC connection: " << ex;
        throw;
    }
}

void Pool::RunSizeMonitor() {
    if (AliveConnectionsCountApprox() >= min_pool_size_) {
        return;
    }
    try {
        PushConnection(engine::Deadline::FromDuration(kConnectionSetupTimeout));
    } catch (const std::exception& ex) {
        LOG_WARNING() << "Failed to restore ODBC min_pool_size: " << ex;
    } catch (...) {
        LOG_WARNING() << "Failed to restore ODBC min_pool_size due to an unknown error";
    }
}

void Pool::AccountConnectionCreated() noexcept { ++stats_.connection.active; }

void Pool::AccountConnectionAcquired() noexcept {}

void Pool::AccountConnectionReleased() noexcept {}

void Pool::AccountConnectionDestroyed() noexcept {
    --stats_.connection.active;
    ++stats_.connection.drop_total;
}

void Pool::AccountOverload() noexcept { ++stats_.pool_exhaust_errors; }

void Pool::AccountQueryExecuted(std::chrono::microseconds duration) noexcept {
    ++stats_.transaction.execute_total;
    stats_.transaction.busy_percentile.Account(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

void Pool::AccountQueryError() noexcept { ++stats_.transaction.error_execute_total; }

void Pool::AccountQueryTimeout() noexcept { ++stats_.transaction.execute_timeout; }

void Pool::AccountOutOfTransaction() noexcept { ++stats_.transaction.out_of_trx_total; }

void Pool::AccountTransactionStarted() noexcept { ++stats_.transaction.total; }

void Pool::AccountTransactionCommit(std::chrono::microseconds total_duration, std::chrono::microseconds busy_duration)
    noexcept {
    ++stats_.transaction.commit_total;
    stats_.transaction.total_percentile
        .Account(std::chrono::duration_cast<std::chrono::milliseconds>(total_duration).count());
    stats_.transaction.busy_percentile
        .Account(std::chrono::duration_cast<std::chrono::milliseconds>(busy_duration).count());
}

void Pool::AccountTransactionRollback() noexcept { ++stats_.transaction.rollback_total; }

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
