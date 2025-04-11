#pragma once

#include <userver/storages/sqlite/infra/strategy/pool_strategy.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::strategy {

class ReadOnlyStrategy final : public PoolStrategyBase {
public:
    ReadOnlyStrategy(const settings::SQLiteSettings& settings, engine::TaskProcessor& blocking_task_processor);
    ~ReadOnlyStrategy() final;

    void WriteStatistics(utils::statistics::Writer& writer) const final;

private:
    Pool& GetReadOnly() const final;
    Pool& GetReadWrite() const final;

    PoolPtr
    InitializeReadOnlyPoolReference(settings::SQLiteSettings settings, engine::TaskProcessor& blocking_task_processor);

    PoolPtr read_connection_pool_;
};

}  // namespace storages::sqlite::infra::strategy

USERVER_NAMESPACE_END
