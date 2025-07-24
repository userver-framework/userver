#include <userver/storages/sqlite/infra/strategy/exclusive_read_write.hpp>

#include <userver/utils/assert.hpp>

#include <userver/storages/sqlite/infra/pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::strategy {

ExclusiveReadWriteStrategy::ExclusiveReadWriteStrategy(
    const settings::SQLiteSettings& settings,
    engine::TaskProcessor& blocking_task_processor
)
    : read_write_connection_pool_{InitializeReadWritePoolReference(settings, blocking_task_processor)} {}

ExclusiveReadWriteStrategy::~ExclusiveReadWriteStrategy() = default;

Pool& ExclusiveReadWriteStrategy::GetReadOnly() const { return *read_write_connection_pool_; }

Pool& ExclusiveReadWriteStrategy::GetReadWrite() const { return *read_write_connection_pool_; }

PoolPtr ExclusiveReadWriteStrategy::InitializeReadWritePoolReference(
    settings::SQLiteSettings settings,
    engine::TaskProcessor& blocking_task_processor
) {
    settings.read_mode = settings::SQLiteSettings::ReadMode::kReadWrite;
    settings.pool_settings.initial_pool_size = 1;
    settings.pool_settings.max_pool_size = 1;

    PoolPtr write_connection_pool;
    engine::TaskWithResult<void> init_task =
        engine::AsyncNoSpan([&write_connection_pool, &blocking_task_processor, &settings]() {
            write_connection_pool = Pool::Create(settings, blocking_task_processor);
        });
    init_task.Get();
    return write_connection_pool;
}

void ExclusiveReadWriteStrategy::WriteStatistics(utils::statistics::Writer& writer) const {
    auto& read_write_stat = read_write_connection_pool_->GetStatistics();

    auto& read_write_connections_stat = read_write_stat.connections;

    auto& read_write_queries_stat = read_write_stat.queries;

    auto& read_write_transactions_stat = read_write_stat.transactions;

    statistics::AggregatedInstanceStatistics instance_stat{
        &read_write_connections_stat,
        &read_write_connections_stat,
        &read_write_queries_stat,
        &read_write_queries_stat,
        &read_write_transactions_stat};
    writer.ValueWithLabels(instance_stat, {});
}

}  // namespace storages::sqlite::infra::strategy

USERVER_NAMESPACE_END
