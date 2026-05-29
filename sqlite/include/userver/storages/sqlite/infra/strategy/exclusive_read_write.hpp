#pragma once

/// @file userver/storages/sqlite/infra/strategy/exclusive_read_write.hpp
/// @brief @copybrief storages::sqlite::infra::strategy::ExclusiveReadWriteStrategy

#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>

#include <userver/storages/sqlite/infra/strategy/pool_strategy.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::strategy {

/// @brief SQLite pool strategy: single shared read-write pool
class ExclusiveReadWriteStrategy final : public PoolStrategyBase {
public:
    ExclusiveReadWriteStrategy(
        const settings::SQLiteSettings& settings,
        engine::TaskProcessor& blocking_task_processor
    );
    ~ExclusiveReadWriteStrategy() final;

    void WriteStatistics(utils::statistics::Writer& writer) const final;

private:
    Pool& GetReadOnly() const final;
    Pool& GetReadWrite() const final;

    PoolPtr InitializeReadWritePoolReference(
        settings::SQLiteSettings settings,
        engine::TaskProcessor& blocking_task_processor
    );

    PoolPtr read_write_connection_pool_;
};

}  // namespace storages::sqlite::infra::strategy

USERVER_NAMESPACE_END
