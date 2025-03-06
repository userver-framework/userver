#include <userver/storages/sqlite/infra/read_only_topology.hpp>

#include <userver/storages/sqlite/infra/pool.hpp>
#include <userver/utils/assert.hpp>
#include "userver/storages/sqlite/options.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra {

ReadOnly::ReadOnly(const settings::SQLiteSettings& settings,
                   engine::TaskProcessor& blocking_task_processor)
    : read_connection_pool_{
          InitializeReadOnlyPoolReference(settings, blocking_task_processor)} {}

ReadOnly::~ReadOnly() = default;

Pool& ReadOnly::GetReadOnly() const { return *read_connection_pool_; }

Pool& ReadOnly::GetReadWrite() const { return GetReadOnly(); }

PoolPtr ReadOnly::InitializeReadOnlyPoolReference(
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

}  // namespace storages::sqlite::infra

USERVER_NAMESPACE_END
