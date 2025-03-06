#include <userver/storages/sqlite/connection.hpp>

#include <optional>

#include <sqlite3.h>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/infra/pool.hpp>
#include "userver/storages/sqlite/infra/connection_ptr.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

std::unique_ptr<infra::TopologyBase> CreateTopology(
    const settings::SQLiteSettings& settings,
    engine::TaskProcessor& blocking_task_processor) {
  return infra::TopologyBase::Create(settings, blocking_task_processor);
}

Connection::Connection(const settings::SQLiteSettings& settings,
                       engine::TaskProcessor& blocking_task_processor)
    : topology_(CreateTopology(settings, blocking_task_processor)) {}

Connection::~Connection() = default;

Transaction Connection::Begin(
    std::string name, const settings::TransactionOptions& options) const {
  return Begin(std::nullopt, name, options);
}

Transaction Connection::Begin(
    settings::OptionalCommandControl optional_cc,
    std::string name [[maybe_unused]],
    const settings::TransactionOptions& options) const {
  if (!optional_cc.has_value()) {
    optional_cc = settings::CommandControl::GetDefault();
  }
  auto connection =
      topology_->SelectPool(optional_cc->operation_type).Acquire();
  return Transaction{std::move(connection), options};
}

Savepoint Connection::Save(std::string name) const {
  return Save(std::nullopt, name);
}

Savepoint Connection::Save(settings::OptionalCommandControl optional_cc,
                           std::string name) const {
  if (!optional_cc.has_value()) {
    optional_cc = settings::CommandControl::GetDefault();
  }
  auto connection = std::make_shared<infra::ConnectionPtr>(
      topology_->SelectPool(optional_cc->operation_type).Acquire());
  return Savepoint{connection, std::move(name)};
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
