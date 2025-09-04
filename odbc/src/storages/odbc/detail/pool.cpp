#include <storages/odbc/detail/pool.hpp>

#include <userver/logging/log.hpp>
#include <userver/storages/odbc/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

namespace {

auto constexpr kInitTimeout = std::chrono::milliseconds{1000};

}  // namespace

Pool::Pool(const std::string& dsn, std::size_t max_pool_size, std::size_t max_simultaneously_connecting_clients)
    : ConnectionPoolBase<Connection, Pool>(max_pool_size, max_simultaneously_connecting_clients), dsn_(dsn) {
    try {
        Init(0, kInitTimeout);
    } catch (const Error& odbcErr) {
        Reset();
        throw;
    } catch (const std::exception& ex) {
        LOG_ERROR() << "Error while initializing ODBC connection pool: " << ex;
        throw;
    }
}

Pool::~Pool() { Reset(); }

ConnectionPtr Pool::Acquire() {
    auto connWrapper = AcquireConnection({});

    return {std::move(connWrapper.pool_ptr), std::move(connWrapper.connection_ptr)};
}

void Pool::Release(ConnectionUniquePtr connection) { ReleaseConnection(std::move(connection)); }

Pool::ConnectionUniquePtr Pool::DoCreateConnection(engine::Deadline deadline) {
    if (deadline.IsReached()) {
        throw std::runtime_error("Connection creation deadline reached");
    }

    try {
        return std::make_unique<Connection>(dsn_);
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
