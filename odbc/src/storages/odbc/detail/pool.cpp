#include <storages/odbc/detail/pool.hpp>

#include <userver/logging/log.hpp>
#include <userver/storages/odbc/exception.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

namespace {

auto constexpr kInitTimeout = std::chrono::milliseconds{1000};

}  // namespace

Pool::Pool(const std::string& dsn, std::size_t min_pool_size, std::size_t max_pool_size)
    : ConnectionPoolBase<Connection, Pool>(max_pool_size, max_pool_size),
      dsns_({dsn}),
      max_pool_size_(max_pool_size)
{
    stats_.connection.maximum = max_pool_size;
    try {
        Init(min_pool_size, kInitTimeout);
    } catch (const Error& odbc_err) {
        Reset();
        throw;
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while initializing ODBC connection pool: " << ex;
        throw;
    }
}

Pool::Pool(std::vector<std::string> dsns, std::size_t min_pool_size, std::size_t max_pool_size)
    : ConnectionPoolBase<Connection, Pool>(max_pool_size, max_pool_size),
      dsns_(std::move(dsns)),
      max_pool_size_(max_pool_size)
{
    stats_.connection.maximum = max_pool_size;
    try {
        Init(min_pool_size, kInitTimeout);
    } catch (const Error& odbc_err) {
        Reset();
        throw;
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while initializing ODBC connection pool: " << ex;
        throw;
    }
}

Pool::~Pool() { Reset(); }

ConnectionPtr Pool::Acquire(engine::Deadline deadline) {
    const auto start = utils::datetime::SteadyCoarseClock::now();
    ++stats_.connection.waiting;

    auto conn_wrapper = AcquireConnection(deadline);

    --stats_.connection.waiting;
    ++stats_.connection.used;

    const auto elapsed = std::chrono::duration_cast<
        std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - start);
    stats_.acquire_percentile.Account(elapsed.count());

    return {std::move(conn_wrapper.pool_ptr), std::move(conn_wrapper.connection_ptr)};
}

void Pool::Release(ConnectionUniquePtr connection) {
    --stats_.connection.used;
    ReleaseConnection(std::move(connection));
}

Pool::ConnectionUniquePtr Pool::DoCreateConnection(engine::Deadline deadline) {
    if (deadline.IsReached()) {
        ++stats_.connection.error_timeout;
        throw std::runtime_error("Connection creation deadline reached");
    }

    const auto start = utils::datetime::SteadyCoarseClock::now();
    try {
        const auto idx = dsn_index_.fetch_add(1);
        auto conn = std::make_unique<Connection>(dsns_[idx % dsns_.size()]);
        ++stats_.connection.open_total;

        const auto elapsed = std::chrono::duration_cast<
            std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - start);
        stats_.connection_percentile.Account(elapsed.count());

        return conn;
    } catch (const std::exception& ex) {
        ++stats_.connection.error_total;
        LOG_ERROR() << "Failed to create ODBC connection: " << ex;
        throw;
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
    stats_.transaction.busy_percentile.Account(duration.count());
}

void Pool::AccountQueryError() noexcept { ++stats_.transaction.error_execute_total; }

void Pool::AccountQueryTimeout() noexcept { ++stats_.transaction.execute_timeout; }

void Pool::AccountOutOfTransaction() noexcept { ++stats_.transaction.out_of_trx_total; }

void Pool::AccountTransactionStarted() noexcept { ++stats_.transaction.total; }

void Pool::AccountTransactionCommit(std::chrono::microseconds total_duration, std::chrono::microseconds busy_duration)
    noexcept {
    ++stats_.transaction.commit_total;
    stats_.transaction.total_percentile.Account(total_duration.count());
    stats_.transaction.busy_percentile.Account(busy_duration.count());
}

void Pool::AccountTransactionRollback() noexcept { ++stats_.transaction.rollback_total; }

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
