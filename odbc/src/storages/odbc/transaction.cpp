#include <userver/storages/odbc/transaction.hpp>

#include <algorithm>
#include <storages/odbc/detail/bulk.hpp>
#include <storages/odbc/detail/conn_ptr.hpp>
#include <storages/odbc/detail/connection.hpp>
#include <storages/odbc/detail/deadline.hpp>
#include <storages/odbc/detail/pool.hpp>
#include <storages/odbc/detail/statement_stats.hpp>
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
    : Transaction{
          std::move(connection),
          pool,
          TransactionOptions{},
          network_timeout,
          statement_timeout,
      } {}

Transaction::Transaction(
    detail::ConnectionPtr&& connection,
    detail::Pool& pool,
    const TransactionOptions& options,
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
    (*connection_)->Begin(options, deadline);
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

ResultSet Transaction::Execute(const Query& query, const ParameterStore& store) {
    return Execute(std::nullopt, query, store);
}

ResultSet Transaction::Execute(
    OptionalCommandControl command_control,
    const Query& query,
    const ParameterStore& store
) {
    return DoExecute(command_control, query, store.GetParameters());
}

BulkResult Transaction::ExecuteBulk(const Query& query, const BulkParameterStore& rows, std::size_t chunk_rows) {
    return ExecuteBulk(std::nullopt, query, rows, chunk_rows);
}

BulkResult Transaction::ExecuteBulk(
    OptionalCommandControl command_control,
    const Query& query,
    const BulkParameterStore& rows,
    std::size_t chunk_rows
) {
    AssertValid();
    if (chunk_rows == 0) {
        throw LogicError("ODBC bulk chunk size must be greater than zero");
    }
    if (rows.IsEmpty()) {
        return {};
    }
    const auto layout = detail::ValidateBulkRows(rows.GetRows());
    return DoExecuteBulk(command_control, query, rows.GetRows(), layout, chunk_rows);
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

    const auto resolved =
        (*connection_)
            ->ResolveTransactionCommandControl(
                CommandControl{.network_timeout = network_timeout_, .statement_timeout = statement_timeout_},
                query,
                command_control
            );
    const auto network_timeout = resolved.network_timeout.value_or(network_timeout_);
    const auto statement_timeout = resolved.statement_timeout.value_or(statement_timeout_);
    const auto statement_deadline =
        std::min(detail::GetExecuteDeadline(network_timeout), detail::GetExecuteDeadline(statement_timeout));

    const auto start = utils::datetime::SteadyCoarseClock::now();
    detail::StatementStats statement_stats{query, pool_->GetStatementStatsStorage()};
    try {
        detail::CheckDeadlineNotExpired(statement_deadline);
        auto result = (*connection_)->Query(query, parameters, statement_deadline);
        const auto elapsed = std::chrono::duration_cast<
            std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - start);
        busy_time_ += elapsed;
        pool_->AccountQueryExecuted(elapsed);
        statement_stats.AccountSuccess();
        return result;
    } catch (const OperationInterrupted&) {
        statement_stats.AccountError();
        pool_->AccountQueryTimeout();
        throw;
    } catch (const Error&) {
        statement_stats.AccountError();
        pool_->AccountQueryError();
        throw;
    } catch (const std::exception&) {
        statement_stats.AccountError();
        throw;
    }
}

BulkResult Transaction::DoExecuteBulk(
    OptionalCommandControl command_control,
    const Query& query,
    const impl::ParameterRows& rows,
    const detail::BulkLayout& layout,
    std::size_t chunk_rows
) {
    AssertValid();
    tracing::Span span{storages::odbc::impl::tracing::kExecuteSpan};

    const auto resolved =
        (*connection_)
            ->ResolveTransactionCommandControl(
                CommandControl{.network_timeout = network_timeout_, .statement_timeout = statement_timeout_},
                query,
                command_control
            );
    const auto network_timeout = resolved.network_timeout.value_or(network_timeout_);
    const auto statement_timeout = resolved.statement_timeout.value_or(statement_timeout_);
    const auto statement_deadline =
        std::min(detail::GetExecuteDeadline(network_timeout), detail::GetExecuteDeadline(statement_timeout));

    const auto start = utils::datetime::SteadyCoarseClock::now();
    detail::StatementStats statement_stats{query, pool_->GetStatementStatsStorage()};
    try {
        detail::CheckDeadlineNotExpired(statement_deadline);
        auto result = (*connection_)->QueryBulk(query, rows, layout, chunk_rows, statement_deadline);
        const auto elapsed = std::chrono::duration_cast<
            std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - start);
        busy_time_ += elapsed;
        pool_->AccountQueryExecuted(elapsed);
        statement_stats.AccountSuccess();
        return result;
    } catch (const OperationInterrupted&) {
        statement_stats.AccountError();
        pool_->AccountQueryTimeout();
        throw;
    } catch (const Error&) {
        statement_stats.AccountError();
        pool_->AccountQueryError();
        throw;
    } catch (const std::exception&) {
        statement_stats.AccountError();
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
