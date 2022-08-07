#include "connection_ptr.hpp"

#include <urabbitmq/connection.hpp>
#include <urabbitmq/connection_pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

ConnectionPtr::ConnectionPtr(std::shared_ptr<ConnectionPool>&& pool,
                             std::unique_ptr<Connection>&& conn)
    : pool_{std::move(pool)}, conn_{std::move(conn)} {}

ConnectionPtr::~ConnectionPtr() { Release(); }

ConnectionPtr::ConnectionPtr(ConnectionPtr&& other) noexcept = default;

Connection* ConnectionPtr::operator->() { return conn_.get(); }

void ConnectionPtr::Adopt() { pool_.reset(); }

void ConnectionPtr::Release() {
  if (pool_ && conn_) {
    pool_->Release(std::move(conn_));
  }
}

}  // namespace urabbitmq

USERVER_NAMESPACE_END
