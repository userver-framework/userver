#include <userver/storages/sqlite/infra/topology_base.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/wait_all_checked.hpp>

#include <userver/storages/sqlite/infra/pool.hpp>
#include "userver/storages/sqlite/infra/read_only_topology.hpp"
#include "userver/storages/sqlite/infra/read_write_topology.hpp"
#include "userver/storages/sqlite/options.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::infra {

TopologyBase::~TopologyBase() = default;

std::unique_ptr<TopologyBase> TopologyBase::Create(
    const settings::SQLiteSettings& settings,
    engine::TaskProcessor& blocking_task_processor) {
  if (settings.read_mode == settings::SQLiteSettings::ReadMode::kReadOnly) {
    return std::make_unique<infra::ReadOnly>(settings, blocking_task_processor);
  } else {
    return std::make_unique<infra::ReadWrite>(settings,
                                              blocking_task_processor);
  }
}

Pool& TopologyBase::SelectPool(
    settings::CommandControl::OperationType op_type) const {
  switch (op_type) {
    case settings::CommandControl::OperationType::kReadOnly:
      return GetReadOnly();
    case settings::CommandControl::OperationType::kReadWrite:
      return GetReadWrite();
  }

  UINVARIANT(false, "Unknown host type");
}

}  // namespace storages::sqlite::infra

USERVER_NAMESPACE_END
