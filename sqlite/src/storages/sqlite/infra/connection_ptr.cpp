#include <userver/storages/sqlite/infra/connection_ptr.hpp>

#include <userver/utils/assert.hpp>

#include <userver/storages/sqlite/impl/connection.hpp>
#include <userver/storages/sqlite/infra/pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra {

ConnectionPtr::ConnectionPtr(std::shared_ptr<Pool>&& pool, std::unique_ptr<impl::Connection>&& connection)
    : pool_{std::move(pool)}, conn_{std::move(connection)} {}

ConnectionPtr::~ConnectionPtr() { Reset(nullptr, nullptr); }

ConnectionPtr::ConnectionPtr(ConnectionPtr&& other) noexcept { Reset(std::move(other.conn_), std::move(other.pool_)); }

ConnectionPtr& ConnectionPtr::operator=(ConnectionPtr&& other) noexcept {
    Reset(std::move(other.conn_), std::move(other.pool_));
    return *this;
}

bool ConnectionPtr::IsValid() const noexcept { return conn_ != nullptr; }

impl::Connection* ConnectionPtr::get() const noexcept { return conn_.get(); }

impl::Connection& ConnectionPtr::operator*() const {
    UASSERT_MSG(conn_, "Dereferencing null connection pointer");
    return *conn_;
}

impl::Connection* ConnectionPtr::operator->() const noexcept { return conn_.get(); }

void ConnectionPtr::Reset(std::unique_ptr<impl::Connection> conn, std::shared_ptr<Pool> pool) {
    Release();
    conn_ = std::move(conn);
    pool_ = std::move(pool);
}

void ConnectionPtr::Release() {
    if (!pool_) return;

    pool_->Release(std::move(conn_));
}

}  // namespace storages::sqlite::infra

USERVER_NAMESPACE_END
