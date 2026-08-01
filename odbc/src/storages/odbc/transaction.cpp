#include <userver/storages/odbc/transaction.hpp>

#include <algorithm>
#include <storages/odbc/detail/conn_ptr.hpp>
#include <storages/odbc/detail/connection.hpp>
#include <storages/odbc/detail/deadline.hpp>
#include <storages/odbc/detail/pool.hpp>
#include <userver/storages/odbc/exception.hpp>
#include <userver/storages/odbc/impl/tracing_tags.hpp>

#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

Transaction::Transaction(
    detail::ConnectionPtr&& connection,
    detail::Pool& pool,
    std::chrono::milliseconds network_timeout,
    std::chrono::milliseconds statement_timeout
)
    : connection_{std::move(connection)},
      pool_{&pool},
      network_timeout_{network_timeout},
      statement_timeout_{statement_timeout},
      start_time_{utils::datetime::SteadyCoarseClock::now()},
      busy_time_{0},
      span_{storages::odbc::impl::tracing::kTransactionSpan}
{
    const auto deadline =
        std::min(detail::GetExecuteDeadline(network_timeout_), detail::GetExecuteDeadline(statement_timeout_));
    detail::CheckDeadlineNotExpired(deadline);
    (*connection_)->Begin(deadline);
    trx_lock_.Lock();
    pool_->AccountTransactionStarted();
}

Transaction::Transaction(Transaction&& other) noexcept = default;

Transaction::~Transaction() {
    if (connection_->IsValid()) {
        const engine::TaskCancellationBlocker cancellation_blocker;
        try {
            (*connection_)->Rollback(engine::Deadline::FromDuration(detail::kDefaultCleanupTimeout));
            pool_->AccountTransactionRollback();
        } catch (const std::exception& ex) {
            (*connection_)->NotifyBroken();
            LOG_ERROR() << "Failed to auto rollback a transaction: " << ex.what();
        } catch (...) {
            (*connection_)->NotifyBroken();
            LOG_ERROR() << "Failed to auto rollback a transaction with an unknown exception";
        }
        trx_lock_.Unlock();
    }
}

void Transaction::Commit() {
    const utils::FastScopeGuard unlock_guard([this]() noexcept { trx_lock_.Unlock(); });
    AssertValid();
    const auto deadline =
        std::min(detail::GetExecuteDeadline(network_timeout_), detail::GetExecuteDeadline(statement_timeout_));
    detail::CheckDeadlineNotExpired(deadline);
    auto connection = std::move(*connection_);
    connection->Commit(deadline);

    const auto total_duration = std::chrono::duration_cast<
        std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - start_time_);
    pool_->AccountTransactionCommit(total_duration, busy_time_);
}

void Transaction::Rollback() {
    const utils::FastScopeGuard unlock_guard([this]() noexcept { trx_lock_.Unlock(); });
    AssertValid();
    const auto deadline = engine::Deadline::FromDuration(detail::kDefaultCleanupTimeout);
    auto connection = std::move(*connection_);
    connection->Rollback(deadline);
    pool_->AccountTransactionRollback();
}

ResultSet Transaction::DoExecute(
    OptionalCommandControl command_control,
    const Query& query,
    const impl::ParameterList& parameters
) {
    AssertValid();
    tracing::Span span{storages::odbc::impl::tracing::kExecuteSpan};

    auto network_timeout = network_timeout_;
    auto statement_timeout = statement_timeout_;
    if (command_control) {
        if (command_control->network_timeout) {
            network_timeout = *command_control->network_timeout;
        }
        if (command_control->statement_timeout) {
            statement_timeout = *command_control->statement_timeout;
        }
    }
    const auto statement_deadline =
        std::min(detail::GetExecuteDeadline(network_timeout), detail::GetExecuteDeadline(statement_timeout));
    detail::CheckDeadlineNotExpired(statement_deadline);

    const auto start = utils::datetime::SteadyCoarseClock::now();
    try {
        auto result = (*connection_)->Query(query, parameters, statement_deadline);
        const auto elapsed = std::chrono::duration_cast<
            std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - start);
        busy_time_ += elapsed;
        pool_->AccountQueryExecuted(elapsed);
        return result;
    } catch (const OperationInterrupted&) {
        pool_->AccountQueryTimeout();
        throw;
    } catch (const Error&) {
        pool_->AccountQueryError();
        throw;
    }
}

void Transaction::AssertValid() const {
    if (!connection_->IsValid()) {
        throw TransactionException("Transaction accessed after it's been committed (or rolled back)");
    }
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
