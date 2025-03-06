#pragma once

#include <userver/storages/sqlite/infra/topology_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra {

class ReadOnly final : public TopologyBase {
 public:
  ReadOnly(const settings::SQLiteSettings& settings,
           engine::TaskProcessor& blocking_task_processor);
  ~ReadOnly() final;

 private:
  Pool& GetReadOnly() const final;
  Pool& GetReadWrite() const final;

  PoolPtr InitializeReadOnlyPoolReference(
      settings::SQLiteSettings settings,
      engine::TaskProcessor& blocking_task_processor);

  PoolPtr read_connection_pool_;
};

}  // namespace storages::sqlite::infra

USERVER_NAMESPACE_END
