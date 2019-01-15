#include <storages/postgres/detail/connection_ptr.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/pool_impl.hpp>

namespace storages {
namespace postgres {
namespace detail {

ConnectionPtr::ConnectionPtr(std::nullptr_t) {}

ConnectionPtr::ConnectionPtr(std::unique_ptr<Connection> conn)
    : conn_(std::move(conn)) {}

ConnectionPtr::ConnectionPtr(Connection* conn,
                             std::shared_ptr<ConnectionPoolImpl> pool)
    : conn_(conn), pool_(std::move(pool)) {
  assert(pool_ && "This constructor requires non-empty parent pool");
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
  assert(conn_ && "Dereferencing null connection pointer");
  return *conn_;
}

Connection* ConnectionPtr::operator->() const noexcept { return conn_.get(); }

void ConnectionPtr::Reset(std::unique_ptr<Connection> conn,
                          std::shared_ptr<ConnectionPoolImpl> pool) {
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

}  // namespace detail
}  // namespace postgres
}  // namespace storages
