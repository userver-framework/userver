#pragma once

#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>

#include <userver/storages/sqlite/infra/strategy/pool_strategy.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::strategy {

class ExclusiveReadWriteStrategy final : public PoolStrategyBase {
 public:
  ExclusiveReadWriteStrategy(const settings::SQLiteSettings& settings,
                             engine::TaskProcessor& blocking_task_processor);
  ~ExclusiveReadWriteStrategy() final;

 private:
  Pool& GetReadOnly() const final;
  Pool& GetReadWrite() const final;

  PoolPtr InitializeReadOnlyPoolReference(
      settings::SQLiteSettings settings,
      engine::TaskProcessor& blocking_task_processor);

  PoolPtr InitializeReadWritePoolReference(
      settings::SQLiteSettings settings,
      engine::TaskProcessor& blocking_task_processor);

  // Order is strong, write connection would be create first
  PoolPtr write_connection_pool_;
  PoolPtr read_connection_pool_;
};

}  // namespace storages::sqlite::infra::strategy

USERVER_NAMESPACE_END
