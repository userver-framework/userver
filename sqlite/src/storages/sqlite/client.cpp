#include <userver/storages/sqlite/client.hpp>

#include <memory>

#include <userver/storages/sqlite/impl/client_impl.hpp>
#include <userver/storages/sqlite/infra/pool.hpp>
#include <userver/storages/sqlite/infra/statistics/statistics_counter.hpp>
#include <userver/storages/sqlite/infra/strategy/pool_strategy.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Client::Client(const settings::SQLiteSettings& settings, engine::TaskProcessor& blocking_task_processor)
    : pimpl_(std::make_unique<impl::ClientImpl>(settings, blocking_task_processor)) {}

Client::~Client() = default;

std::shared_ptr<infra::ConnectionPtr> Client::GetConnection(OperationType operation_type) const {
    return pimpl_->GetConnection(operation_type);
}

Transaction Client::Begin(OperationType operation_type, const settings::TransactionOptions& options) const {
    auto connection = GetConnection(operation_type);
    return Transaction{std::move(connection), options};
}

Savepoint Client::Save(OperationType operation_type, std::string name) const {
    auto connection = GetConnection(operation_type);
    return Savepoint{std::move(connection), std::move(name)};
}

ResultSet Client::DoExecute(impl::io::ParamsBinderBase& params, std::shared_ptr<infra::ConnectionPtr> connection)
    const {
    auto prepare_statement = params.GetBindsPtr();
    return pimpl_->ExecuteCommand(prepare_statement, connection);
}

void Client::WriteStatistics(utils::statistics::Writer& writer) const { return pimpl_->WriteStatistics(writer); }

void Client::AccountQueryExecute(std::shared_ptr<infra::ConnectionPtr> connection) const noexcept {
    pimpl_->AccountQueryExecute(connection);
}

void Client::AccountQueryFailed(std::shared_ptr<infra::ConnectionPtr> connection) const noexcept {
    pimpl_->AccountQueryFailed(connection);
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
