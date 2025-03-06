#include <userver/storages/sqlite/infra/strategy/pool_strategy.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/wait_all_checked.hpp>

#include <userver/storages/sqlite/infra/pool.hpp>
#include <userver/storages/sqlite/infra/strategy/read_only_strategy.hpp>
#include <userver/storages/sqlite/infra/strategy/read_write_strategy.hpp>
#include <userver/storages/sqlite/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::strategy {

PoolStrategyBase::~PoolStrategyBase() = default;

std::unique_ptr<PoolStrategyBase> PoolStrategyBase::Create(
    const settings::SQLiteSettings& settings,
    engine::TaskProcessor& blocking_task_processor) {
  if (settings.read_mode == settings::SQLiteSettings::ReadMode::kReadOnly) {
    return std::make_unique<ReadOnlyStrategy>(settings,
                                              blocking_task_processor);
  } else {
    return std::make_unique<ReadWriteStrategy>(settings,
                                               blocking_task_processor);
  }
}

Pool& PoolStrategyBase::SelectPool(
    settings::CommandControl::OperationType op_type) const {
  switch (op_type) {
    case settings::CommandControl::OperationType::kReadOnly:
      return GetReadOnly();
    case settings::CommandControl::OperationType::kReadWrite:
      return GetReadWrite();
  }

  UINVARIANT(false, "Unknown host type");
}

}  // namespace storages::sqlite::infra::strategy

USERVER_NAMESPACE_END
