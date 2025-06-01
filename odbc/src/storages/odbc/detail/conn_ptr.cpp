#include <storages/odbc/detail/conn_ptr.hpp>

#include <userver/utils/assert.hpp>

#include <storages/odbc/detail/pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

ConnectionPtr::ConnectionPtr(std::shared_ptr<Pool>&& pool, std::unique_ptr<Connection>&& connection)
    : pool_{std::move(pool)}, conn_{std::move(connection)} {}

ConnectionPtr::~ConnectionPtr() { Reset(nullptr, nullptr); }

ConnectionPtr::ConnectionPtr(ConnectionPtr&& other) noexcept { Reset(std::move(other.conn_), std::move(other.pool_)); }

ConnectionPtr& ConnectionPtr::operator=(ConnectionPtr&& other) noexcept {
    Reset(std::move(other.conn_), std::move(other.pool_));
    return *this;
}

bool ConnectionPtr::IsValid() const noexcept { return conn_ != nullptr; }

Connection* ConnectionPtr::get() const noexcept { return conn_.get(); }

Connection& ConnectionPtr::operator*() const {
    UASSERT_MSG(conn_, "Dereferencing null connection pointer");
    return *conn_;
}

Connection* ConnectionPtr::operator->() const noexcept { return conn_.get(); }

void ConnectionPtr::Reset(std::unique_ptr<Connection> conn, std::shared_ptr<Pool> pool) {
    Release();
    conn_ = std::move(conn);
    pool_ = std::move(pool);
}

void ConnectionPtr::Release() {
    if (!pool_) return;

    if (conn_) {
        pool_->Release(std::move(conn_));
    }
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
