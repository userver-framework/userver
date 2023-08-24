#include "connection_ptr.hpp"

#include <storages/clickhouse/impl/connection.hpp>
#include <storages/clickhouse/impl/pool_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

ConnectionPtr::ConnectionPtr(std::shared_ptr<PoolImpl>&& pool, Connection* conn)
    : pool_{std::move(pool)}, conn_{conn} {}

ConnectionPtr::~ConnectionPtr() noexcept { Release(); }

ConnectionPtr::ConnectionPtr(ConnectionPtr&& other) noexcept = default;

Connection& ConnectionPtr::operator*() const { return *conn_; }

Connection* ConnectionPtr::operator->() const noexcept { return conn_.get(); }

void ConnectionPtr::Release() noexcept {
  if (!conn_) return;

  pool_->Release(conn_.release());
}

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
