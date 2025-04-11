#pragma once

#include <userver/storages/sqlite/infra/strategy/pool_strategy.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::strategy {

class ReadWriteStrategy final : public PoolStrategyBase {
public:
    ReadWriteStrategy(const settings::SQLiteSettings& settings, engine::TaskProcessor& blocking_task_processor);
    ~ReadWriteStrategy() final;

    void WriteStatistics(utils::statistics::Writer& writer) const final;

private:
    Pool& GetReadOnly() const final;
    Pool& GetReadWrite() const final;

    PoolPtr
    InitializeReadOnlyPoolReference(settings::SQLiteSettings settings, engine::TaskProcessor& blocking_task_processor);

    PoolPtr
    InitializeReadWritePoolReference(settings::SQLiteSettings settings, engine::TaskProcessor& blocking_task_processor);

    // Order is strong, write connection would be create first
    PoolPtr write_connection_pool_;
    PoolPtr read_connection_pool_;
};

}  // namespace storages::sqlite::infra::strategy

USERVER_NAMESPACE_END
