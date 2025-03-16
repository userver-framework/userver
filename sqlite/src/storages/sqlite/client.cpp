#include <userver/storages/sqlite/client.hpp>

#include <userver/storages/sqlite/impl/client_impl.hpp>
#include <userver/storages/sqlite/infra/pool.hpp>
#include <userver/storages/sqlite/infra/strategy/pool_strategy.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Client::Client(const settings::SQLiteSettings& settings,
               engine::TaskProcessor& blocking_task_processor)
    : pimpl_(std::make_unique<impl::ClientImpl>(settings,
                                                blocking_task_processor)) {}

Client::~Client() = default;

infra::ConnectionPtr Client::GetConnection(
    settings::CommandControl::OperationType op_type) const {
  return pimpl_->GetConnection(op_type);
}

Transaction Client::Begin(std::string name,
                          const settings::TransactionOptions& options) const {
  return Begin(std::nullopt, name, options);
}

Transaction Client::Begin(settings::OptionalCommandControl optional_cc,
                          std::string name [[maybe_unused]],
                          const settings::TransactionOptions& options) const {
  if (!optional_cc.has_value()) {
    optional_cc = settings::CommandControl::GetDefault();
  }
  auto connection = pimpl_->GetConnection(optional_cc->operation_type);
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
  auto connection = pimpl_->GetConnection(optional_cc->operation_type);
  return Savepoint{std::move(connection), std::move(name)};
}

ResultSet Client::DoExecute(settings::OptionalCommandControl optional_cc,
                            impl::io::ParamsBinderBase& params,
                            const infra::ConnectionPtr& connection) const {
  auto prepare_statement = params.GetBindsPtr();
  return connection->ExecuteCommand(optional_cc, prepare_statement);
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
