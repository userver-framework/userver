#include <storages/postgres/pool.hpp>

#include <error_injection/settings.hpp>
#include <storages/postgres/detail/pool_impl.hpp>

namespace storages::postgres {

ConnectionPool::ConnectionPool(
    Dsn dsn, engine::TaskProcessor& bg_task_processor,
    const PoolSettings& pool_settings, const ConnectionSettings& conn_settings,
    const CommandControl& default_cmd_ctl,
    const testsuite::PostgresControl& testsuite_pg_ctl,
    error_injection::Settings ei_settings) {
  pimpl_ = detail::ConnectionPoolImpl::Create(
      std::move(dsn), bg_task_processor, pool_settings, conn_settings,
      default_cmd_ctl, testsuite_pg_ctl, std::move(ei_settings));
}

ConnectionPool::~ConnectionPool() = default;

ConnectionPool::ConnectionPool(ConnectionPool&&) noexcept = default;

ConnectionPool& ConnectionPool::operator=(ConnectionPool&&) noexcept = default;

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

}  // namespace storages::postgres
