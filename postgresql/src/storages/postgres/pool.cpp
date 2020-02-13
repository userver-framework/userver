#include <storages/postgres/pool.hpp>

#include <error_injection/settings.hpp>
#include <storages/postgres/detail/pool_impl.hpp>

namespace storages {
namespace postgres {

ConnectionPool::ConnectionPool(
    const std::string& dsn, engine::TaskProcessor& bg_task_processor,
    PoolSettings pool_settings, ConnectionSettings conn_settings,
    CommandControl default_cmd_ctl,
    const testsuite::PostgresControl& testsuite_pg_ctl,
    const error_injection::Settings& ei_settings) {
  pimpl_ = detail::ConnectionPoolImpl::Create(
      dsn, bg_task_processor, pool_settings, conn_settings, default_cmd_ctl,
      testsuite_pg_ctl, ei_settings);
}

ConnectionPool::~ConnectionPool() = default;

ConnectionPool::ConnectionPool(ConnectionPool&&) noexcept = default;

ConnectionPool& ConnectionPool::operator=(ConnectionPool&&) noexcept = default;

const std::string& ConnectionPool::GetDsn() const { return pimpl_->GetDsn(); }

detail::ConnectionPtr ConnectionPool::GetConnection(engine::Deadline deadline) {
  return pimpl_->Acquire(deadline);
}

const InstanceStatistics& ConnectionPool::GetStatistics() const {
  return pimpl_->GetStatistics();
}

Transaction ConnectionPool::Begin(const TransactionOptions& options,
                                  engine::Deadline deadline,
                                  OptionalCommandControl cmd_ctl) {
  return pimpl_->Begin(options, deadline, cmd_ctl);
}

detail::NonTransaction ConnectionPool::Start(engine::Deadline deadline) {
  return pimpl_->Start(deadline);
}

void ConnectionPool::SetDefaultCommandControl(CommandControl cmd_ctl) {
  pimpl_->SetDefaultCommandControl(cmd_ctl);
}

}  // namespace postgres
}  // namespace storages
