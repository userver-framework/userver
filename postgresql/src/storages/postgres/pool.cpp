#include <storages/postgres/pool.hpp>

#include <storages/postgres/detail/pool_impl.hpp>

namespace storages {
namespace postgres {

ConnectionPool::ConnectionPool(const std::string& dsn,
                               engine::TaskProcessor& bg_task_processor,
                               size_t min_size, size_t max_size) {
  pimpl_ = std::make_shared<detail::ConnectionPoolImpl>(dsn, bg_task_processor,
                                                        min_size, max_size);
}

ConnectionPool::~ConnectionPool() = default;

ConnectionPool::ConnectionPool(ConnectionPool&&) noexcept = default;

ConnectionPool& ConnectionPool::operator=(ConnectionPool&&) = default;

detail::ConnectionPtr ConnectionPool::GetConnection() {
  return pimpl_->Acquire();
}

const InstanceStatistics& ConnectionPool::GetStatistics() const {
  return pimpl_->GetStatistics();
}

Transaction ConnectionPool::Begin(const TransactionOptions& options) {
  return pimpl_->Begin(options);
}

}  // namespace postgres
}  // namespace storages
