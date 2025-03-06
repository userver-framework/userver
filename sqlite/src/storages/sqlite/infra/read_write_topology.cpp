#include <userver/storages/sqlite/infra/read_write_topology.hpp>

#include <userver/storages/sqlite/infra/pool.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra {

ReadWrite::ReadWrite(const settings::SQLiteSettings& settings,
                     engine::TaskProcessor& blocking_task_processor)
    : write_connection_pool_{InitializeReadWritePoolReference(
          settings, blocking_task_processor)},
      read_connection_pool_{
          InitializeReadOnlyPoolReference(settings, blocking_task_processor)} {}

ReadWrite::~ReadWrite() = default;

Pool& ReadWrite::GetReadOnly() const { return *read_connection_pool_; }

Pool& ReadWrite::GetReadWrite() const { return *write_connection_pool_; }

PoolPtr ReadWrite::InitializeReadOnlyPoolReference(
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

PoolPtr ReadWrite::InitializeReadWritePoolReference(
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

}  // namespace storages::sqlite::infra

USERVER_NAMESPACE_END
