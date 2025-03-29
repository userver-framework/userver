#include <userver/storages/sqlite/infra/strategy/read_only.hpp>

#include <userver/utils/assert.hpp>

#include <userver/storages/sqlite/infra/pool.hpp>
#include <userver/storages/sqlite/infra/statistics/statistics.hpp>
#include <userver/storages/sqlite/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::strategy {

ReadOnlyStrategy::ReadOnlyStrategy(
    const settings::SQLiteSettings& settings,
    engine::TaskProcessor& blocking_task_processor)
    : read_connection_pool_{
          InitializeReadOnlyPoolReference(settings, blocking_task_processor)} {}

ReadOnlyStrategy::~ReadOnlyStrategy() = default;

Pool& ReadOnlyStrategy::GetReadOnly() const { return *read_connection_pool_; }

Pool& ReadOnlyStrategy::GetReadWrite() const { return GetReadOnly(); }

PoolPtr ReadOnlyStrategy::InitializeReadOnlyPoolReference(
    settings::SQLiteSettings settings,
    engine::TaskProcessor& blocking_task_processor) {
  settings.read_mode =
      settings::SQLiteSettings::ReadMode::kReadOnly;  // coercively set read
                                                      // only mode
  PoolPtr read_connection_pool;
  engine::TaskWithResult<void> init_task = engine::AsyncNoSpan(
      [&read_connection_pool, &blocking_task_processor, &settings]() {
        read_connection_pool = Pool::Create(settings, blocking_task_processor);
      });
  init_task.Wait();
  return read_connection_pool;
}

void ReadOnlyStrategy::WriteStatistics(
    utils::statistics::Writer& writer) const {
  auto& read_stat = read_connection_pool_->GetStatistics();

  auto read_connections_stat = read_stat.connections;

  auto& read_queries_stat = read_stat.queries;

  statistics::PoolTransactionsStatistics transactions_stat;
  transactions_stat.Add(read_stat.transactions);

  statistics::AgregatedInstanceStatistics instance_stat{
      {}, read_connections_stat, {}, read_queries_stat, transactions_stat};
  writer.ValueWithLabels(instance_stat, {});
}

}  // namespace storages::sqlite::infra::strategy

USERVER_NAMESPACE_END
