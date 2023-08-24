#include <storages/mysql/infra/connection_ptr.hpp>

#include <storages/mysql/impl/connection.hpp>
#include <storages/mysql/infra/pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::infra {

ConnectionPtr::ConnectionPtr(std::shared_ptr<Pool>&& pool,
                             std::unique_ptr<impl::Connection>&& connection)
    : pool_{std::move(pool)}, connection_{std::move(connection)} {}

ConnectionPtr::~ConnectionPtr() { Release(); }

ConnectionPtr::ConnectionPtr(ConnectionPtr&& other) noexcept = default;

impl::Connection& ConnectionPtr::operator*() const { return *connection_; }

impl::Connection* ConnectionPtr::operator->() const noexcept {
  return connection_.get();
}

bool ConnectionPtr::IsValid() const { return connection_ != nullptr; }

void ConnectionPtr::Release() noexcept {
  if (!pool_) return;

  pool_->Release(std::move(connection_));
}

}  // namespace storages::mysql::infra

USERVER_NAMESPACE_END
