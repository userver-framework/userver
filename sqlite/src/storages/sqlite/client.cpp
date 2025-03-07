#include <userver/storages/sqlite/client.hpp>

#include <optional>

#include <sqlite3.h>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/infra/pool.hpp>
#include <userver/storages/sqlite/infra/strategy/pool_strategy.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

std::unique_ptr<infra::strategy::PoolStrategyBase> CreatePoolStrategy(
    const settings::SQLiteSettings& settings,
    engine::TaskProcessor& blocking_task_processor) {
  return infra::strategy::PoolStrategyBase::Create(settings,
                                                   blocking_task_processor);
}

Client::Client(const settings::SQLiteSettings& settings,
               engine::TaskProcessor& blocking_task_processor)
    : pool_strategy_(CreatePoolStrategy(settings, blocking_task_processor)) {}

Client::~Client() = default;

Transaction Client::Begin(std::string name,
                          const settings::TransactionOptions& options) const {
  return Begin(std::nullopt, name, options);
}

infra::ConnectionPtr Client::GetConnection(
    settings::CommandControl::OperationType op_type) const {
  return pool_strategy_->SelectPool(op_type).Acquire();
}

Transaction Client::Begin(settings::OptionalCommandControl optional_cc,
                          std::string name [[maybe_unused]],
                          const settings::TransactionOptions& options) const {
  if (!optional_cc.has_value()) {
    optional_cc = settings::CommandControl::GetDefault();
  }
  auto connection = GetConnection(optional_cc->operation_type);
  return Transaction{std::move(connection), options};
}

Savepoint Client::Save(std::string name) const {
  return Save(std::nullopt, name);
}

Savepoint Client::Save(settings::OptionalCommandControl optional_cc,
                       std::string name) const {
  if (!optional_cc.has_value()) {
    optional_cc = settings::CommandControl::GetDefault();
  }
  auto shared_connection = std::make_shared<infra::ConnectionPtr>(
      GetConnection(optional_cc->operation_type));
  return Savepoint{shared_connection, std::move(name)};
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
