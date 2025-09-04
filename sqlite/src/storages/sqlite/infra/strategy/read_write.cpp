#include <userver/storages/sqlite/infra/strategy/read_write.hpp>

#include <userver/utils/assert.hpp>

#include <userver/storages/sqlite/infra/pool.hpp>
#include <userver/storages/sqlite/infra/statistics/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::strategy {

ReadWriteStrategy::ReadWriteStrategy(
    const settings::SQLiteSettings& settings,
    engine::TaskProcessor& blocking_task_processor
)
    : write_connection_pool_{InitializeReadWritePoolReference(settings, blocking_task_processor)},
      read_connection_pool_{InitializeReadOnlyPoolReference(settings, blocking_task_processor)} {}

ReadWriteStrategy::~ReadWriteStrategy() = default;

Pool& ReadWriteStrategy::GetReadOnly() const { return *read_connection_pool_; }

Pool& ReadWriteStrategy::GetReadWrite() const { return *write_connection_pool_; }

PoolPtr ReadWriteStrategy::InitializeReadOnlyPoolReference(
    settings::SQLiteSettings settings,
    engine::TaskProcessor& blocking_task_processor
) {
    settings.read_mode = settings::SQLiteSettings::ReadMode::kReadOnly;  // coercively set read
                                                                         // only mode
    PoolPtr read_connection_pool;
    engine::TaskWithResult<void> init_task =
        engine::AsyncNoSpan([&read_connection_pool, &blocking_task_processor, &settings]() {
            read_connection_pool = Pool::Create(settings, blocking_task_processor);
        });
    init_task.Get();
    return read_connection_pool;
}

PoolPtr ReadWriteStrategy::InitializeReadWritePoolReference(
    settings::SQLiteSettings settings,
    engine::TaskProcessor& blocking_task_processor
) {
    settings.read_mode = settings::SQLiteSettings::ReadMode::kReadWrite;  // coercively set read
                                                                          // write mode
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

void ReadWriteStrategy::WriteStatistics(utils::statistics::Writer& writer) const {
    auto& write_stat = write_connection_pool_->GetStatistics();
    auto& read_stat = read_connection_pool_->GetStatistics();

    auto& write_connections_stat = write_stat.connections;
    auto& read_connections_stat = read_stat.connections;

    auto& write_queries_stat = write_stat.queries;
    auto& read_queries_stat = read_stat.queries;

    statistics::PoolTransactionsStatisticsAggregated transactions_stat;
    transactions_stat.Add(write_stat.transactions);
    transactions_stat.Add(read_stat.transactions);

    statistics::AggregatedInstanceStatistics instance_stat{
        &write_connections_stat,
        &read_connections_stat,
        &write_queries_stat,
        &read_queries_stat,
        nullptr,
        &transactions_stat};
    writer.ValueWithLabels(instance_stat, {});
}

}  // namespace storages::sqlite::infra::strategy

USERVER_NAMESPACE_END
