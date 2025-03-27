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

infra::ConnectionPtr Client::GetConnection(OperationType operation_type) const {
  return pimpl_->GetConnection(operation_type);
}

Transaction Client::Begin(OperationType operation_type,
                          const settings::TransactionOptions& options) const {
  auto connection = pimpl_->GetConnection(operation_type);
  return Transaction{std::move(connection), options};
}

Savepoint Client::Save(OperationType operation_type, std::string name) const {
  auto connection = pimpl_->GetConnection(operation_type);
  return Savepoint{std::move(connection), std::move(name)};
}

ResultSet Client::DoExecute(impl::io::ParamsBinderBase& params,
                            const infra::ConnectionPtr& connection) const {
  auto prepare_statement = params.GetBindsPtr();
  return connection->ExecuteCommand(prepare_statement);
}

void Client::WriteStatistics(utils::statistics::Writer& writer) const {
  return pimpl_->WriteStatistics(writer);
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
