#include <userver/storages/postgres/detail/connection_ptr.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

ConnectionPtr::ConnectionPtr(std::unique_ptr<Connection>&& conn)
    : conn_(std::move(conn)) {}

ConnectionPtr::ConnectionPtr(Connection* conn,
                             std::shared_ptr<ConnectionPool>&& pool)
    : pool_(std::move(pool)), conn_(conn) {
  UASSERT_MSG(pool_, "This constructor requires non-empty parent pool");
}

ConnectionPtr::~ConnectionPtr() { Reset(nullptr, nullptr); }

ConnectionPtr::ConnectionPtr(ConnectionPtr&& other) noexcept {
  Reset(std::move(other.conn_), std::move(other.pool_));
}

ConnectionPtr& ConnectionPtr::operator=(ConnectionPtr&& other) noexcept {
  Reset(std::move(other.conn_), std::move(other.pool_));
  return *this;
}

ConnectionPtr::operator bool() const noexcept { return conn_ != nullptr; }

Connection* ConnectionPtr::get() const noexcept { return conn_.get(); }

Connection& ConnectionPtr::operator*() const {
  UASSERT_MSG(conn_, "Dereferencing null connection pointer");
  return *conn_;
}

Connection* ConnectionPtr::operator->() const noexcept { return conn_.get(); }

const StatementTimingsStorage* ConnectionPtr::GetStatementTimingsStorage()
    const {
  if (!pool_) return nullptr;

  return &pool_->GetStatementTimingsStorage();
}

void ConnectionPtr::Reset(std::unique_ptr<Connection> conn,
                          std::shared_ptr<ConnectionPool> pool) {
  Release();
  conn_ = std::move(conn);
  pool_ = std::move(pool);
}

void ConnectionPtr::Release() {
  // We release pooled connection but reset standalone one
  if (pool_) {
    pool_->Release(conn_.release());
  } else {
    conn_.reset();
  }
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
