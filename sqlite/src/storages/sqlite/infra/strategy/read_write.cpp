#include <userver/storages/sqlite/infra/strategy/read_write.hpp>

#include <userver/utils/assert.hpp>

#include <userver/storages/sqlite/infra/pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra::strategy {

ReadWriteStrategy::ReadWriteStrategy(
    const settings::SQLiteSettings& settings,
    engine::TaskProcessor& blocking_task_processor)
    : write_connection_pool_{InitializeReadWritePoolReference(
          settings, blocking_task_processor)},
      read_connection_pool_{
          InitializeReadOnlyPoolReference(settings, blocking_task_processor)} {}

ReadWriteStrategy::~ReadWriteStrategy() = default;

Pool& ReadWriteStrategy::GetReadOnly() const { return *read_connection_pool_; }

Pool& ReadWriteStrategy::GetReadWrite() const {
  return *write_connection_pool_;
}

PoolPtr ReadWriteStrategy::InitializeReadOnlyPoolReference(
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
  init_task.Get();
  return read_connection_pool;
}

PoolPtr ReadWriteStrategy::InitializeReadWritePoolReference(
    settings::SQLiteSettings settings,
    engine::TaskProcessor& blocking_task_processor) {
  settings.read_mode =
      settings::SQLiteSettings::ReadMode::kReadWrite;  // coercively set read
                                                       // write mode
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

void ReadWriteStrategy::WriteStatistics(
    utils::statistics::Writer& writer) const {
  auto writer_stat = write_connection_pool_->GetStatistics();
  writer.ValueWithLabels(writer_stat, {{"connection_pool", "write"}});
  auto reader_stat = read_connection_pool_->GetStatistics();
  writer.ValueWithLabels(reader_stat, {{"connection_pool", "read"}});
}

}  // namespace storages::sqlite::infra::strategy

USERVER_NAMESPACE_END
