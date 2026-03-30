#include <storages/odbc/detail/pool.hpp>

#include <userver/logging/log.hpp>
#include <userver/storages/odbc/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

namespace {

auto constexpr kInitTimeout = std::chrono::milliseconds{1000};

}  // namespace

Pool::Pool(const std::string& dsn, std::size_t min_pool_size, std::size_t max_pool_size)
    : ConnectionPoolBase<Connection, Pool>(max_pool_size, max_pool_size),
      dsns_({dsn})
{
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
      dsns_(std::move(dsns))
{
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

ConnectionPtr Pool::Acquire() {
    auto conn_wrapper = AcquireConnection({});

    return {std::move(conn_wrapper.pool_ptr), std::move(conn_wrapper.connection_ptr)};
}

void Pool::Release(ConnectionUniquePtr connection) { ReleaseConnection(std::move(connection)); }

Pool::ConnectionUniquePtr Pool::DoCreateConnection(engine::Deadline deadline) {
    if (deadline.IsReached()) {
        throw std::runtime_error("Connection creation deadline reached");
    }

    try {
        const auto idx = dsn_index_.fetch_add(1, std::memory_order_relaxed);
        return std::make_unique<Connection>(dsns_[idx % dsns_.size()]);
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Failed to create ODBC connection: " << ex;
        throw;
    }
}

void Pool::AccountConnectionCreated() noexcept {}
void Pool::AccountConnectionAcquired() noexcept {}
void Pool::AccountConnectionReleased() noexcept {}
void Pool::AccountConnectionDestroyed() noexcept {}
void Pool::AccountOverload() noexcept {}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
