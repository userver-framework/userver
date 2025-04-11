#include <memory>
#include <userver/storages/sqlite/impl/client_impl.hpp>

#include <userver/storages/sqlite/impl/connection.hpp>
#include <userver/storages/sqlite/infra/pool.hpp>
#include <userver/storages/sqlite/infra/strategy/pool_strategy.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

ClientImpl::ClientImpl(const settings::SQLiteSettings& settings, engine::TaskProcessor& blocking_task_processor) {
    pool_strategy_ = infra::strategy::PoolStrategyBase::Create(settings, blocking_task_processor);
}

ClientImpl::~ClientImpl() = default;

std::shared_ptr<infra::ConnectionPtr> ClientImpl::GetConnection(OperationType operation_type) const {
    return std::make_shared<infra::ConnectionPtr>(pool_strategy_->SelectPool(operation_type).Acquire());
}

void ClientImpl::WriteStatistics(utils::statistics::Writer& writer) const { pool_strategy_->WriteStatistics(writer); }

ResultSet ClientImpl::ExecuteCommand(
    impl::StatementBasePtr prepare_statement,
    std::shared_ptr<infra::ConnectionPtr> connection
) const {
    auto result_wrapper = std::make_unique<impl::ResultWrapper>(prepare_statement, std::move(connection));
    return ResultSet{std::move(result_wrapper)};
}

void ClientImpl::AccountQueryExecute(std::shared_ptr<infra::ConnectionPtr> connection) const noexcept {
    (*connection)->AccountQueryExecute();
}

void ClientImpl::AccountQueryFailed(std::shared_ptr<infra::ConnectionPtr> connection) const noexcept {
    (*connection)->AccountQueryFailed();
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
