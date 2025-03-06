#pragma once

#include <userver/storages/sqlite/infra/topology_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra {

class ReadWrite final : public TopologyBase {
 public:
  ReadWrite(const settings::SQLiteSettings& settings,
            engine::TaskProcessor& blocking_task_processor);
  ~ReadWrite() final;

 private:
  Pool& GetReadOnly() const final;
  Pool& GetReadWrite() const final;

  PoolPtr InitializeReadOnlyPoolReference(
      settings::SQLiteSettings settings,
      engine::TaskProcessor& blocking_task_processor);

  PoolPtr InitializeReadWritePoolReference(
      settings::SQLiteSettings settings,
      engine::TaskProcessor& blocking_task_processor);

  // Order is strong, write connection would be create first
  PoolPtr write_connection_pool_;
  PoolPtr read_connection_pool_;
};

}  // namespace storages::sqlite::infra

USERVER_NAMESPACE_END
