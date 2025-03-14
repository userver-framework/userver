#include <userver/storages/sqlite/impl/client_impl.hpp>

#include <userver/storages/sqlite/impl/connection.hpp>
#include <userver/storages/sqlite/infra/pool.hpp>
#include <userver/storages/sqlite/infra/strategy/pool_strategy.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

ClientImpl::ClientImpl(const settings::SQLiteSettings& settings,
                       engine::TaskProcessor& blocking_task_processor)
    : pool_strategy_(infra::strategy::PoolStrategyBase::Create(
          settings, blocking_task_processor)) {}

ClientImpl::~ClientImpl() = default;

infra::ConnectionPtr ClientImpl::GetConnection(
    settings::CommandControl::OperationType op_type) const {
  return pool_strategy_->SelectPool(op_type).Acquire();
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
