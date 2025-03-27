#include <userver/storages/sqlite/infra/strategy/exclusive_read_write.hpp>

#include <userver/utils/assert.hpp>

#include <userver/storages/sqlite/infra/pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::strategy {

ExclusiveReadWriteStrategy::ExclusiveReadWriteStrategy(
    const settings::SQLiteSettings& settings,
    engine::TaskProcessor& blocking_task_processor)
    : read_write_connection_pool_{InitializeReadWritePoolReference(
          settings, blocking_task_processor)} {}

ExclusiveReadWriteStrategy::~ExclusiveReadWriteStrategy() = default;

Pool& ExclusiveReadWriteStrategy::GetReadOnly() const {
  return *read_write_connection_pool_;
}

Pool& ExclusiveReadWriteStrategy::GetReadWrite() const {
  return *read_write_connection_pool_;
}

PoolPtr ExclusiveReadWriteStrategy::InitializeReadWritePoolReference(
    settings::SQLiteSettings settings,
    engine::TaskProcessor& blocking_task_processor) {
  settings.read_mode = settings::SQLiteSettings::ReadMode::kReadWrite;
  settings.pool_settings.initial_pool_size = 1;
  settings.pool_settings.max_pool_size = 1;

  PoolPtr write_connection_pool;
  engine::TaskWithResult<void> init_task = engine::AsyncNoSpan(
      [&write_connection_pool, &blocking_task_processor, &settings]() {
        write_connection_pool = Pool::Create(settings, blocking_task_processor);
      });
  init_task.Get();
  return write_connection_pool;
}

void ExclusiveReadWriteStrategy::WriteStatistics(
    utils::statistics::Writer& writer) const {
  auto read_writer_stat = read_write_connection_pool_->GetStatistics();
  writer.ValueWithLabels(read_writer_stat, {{"connection_pool", "read_write"}});
}

}  // namespace storages::sqlite::infra::strategy

USERVER_NAMESPACE_END
