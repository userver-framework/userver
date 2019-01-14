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

ConnectionPtr::~ConnectionPtr() {
  // It's possible that connection doesn't have a parent pool if created
  // standalone - in this case it will be freed up automatically
  if (pool_) {
    pool_->Release(conn_.release());
  }
}

ConnectionPtr::ConnectionPtr(ConnectionPtr&&) noexcept = default;

ConnectionPtr& ConnectionPtr::operator=(ConnectionPtr&&) noexcept = default;

ConnectionPtr::operator bool() const noexcept { return conn_ != nullptr; }

Connection* ConnectionPtr::get() const noexcept { return conn_.get(); }

Connection& ConnectionPtr::operator*() const {
  assert(conn_ && "Dereferencing null connection pointer");
  return *conn_;
}

Connection* ConnectionPtr::operator->() const noexcept { return conn_.get(); }

}  // namespace detail
}  // namespace postgres
}  // namespace storages
