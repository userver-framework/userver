#include <userver/storages/odbc/transaction.hpp>

#include <storages/odbc/detail/conn_ptr.hpp>
#include <storages/odbc/detail/connection.hpp>
#include <storages/odbc/detail/deadline.hpp>
#include <storages/odbc/detail/pool.hpp>
#include <userver/storages/odbc/exception.hpp>
#include <userver/storages/odbc/impl/tracing_tags.hpp>

#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

Transaction::Transaction(detail::ConnectionPtr&& connection, detail::Pool& pool, engine::Deadline deadline)
    : connection_{std::move(connection)},
      pool_{&pool},
      deadline_{deadline},
      start_time_{utils::datetime::SteadyCoarseClock::now()},
      busy_time_{0},
      span_{storages::odbc::impl::tracing::kTransactionSpan}
{
    detail::CheckDeadlineNotExpired(deadline_);
    (*connection_)->Begin(deadline_);
    trx_lock_.Lock();
    pool_->AccountTransactionStarted();
}

Transaction::Transaction(Transaction&& other) noexcept = default;

Transaction::~Transaction() {
    if (connection_->IsValid()) {
        try {
            (*connection_)->Rollback(deadline_);
            pool_->AccountTransactionRollback();
        } catch (const std::exception& ex) {
            LOG_ERROR() << "Failed to auto rollback a transaction: " << ex.what();
        }
        trx_lock_.Unlock();
    }
}

void Transaction::Commit() {
    const utils::FastScopeGuard unlock_guard([this]() noexcept { trx_lock_.Unlock(); });
    AssertValid();
    detail::CheckDeadlineNotExpired(deadline_);
    auto connection = std::move(*connection_);
    connection->Commit(deadline_);

    const auto total_duration = std::chrono::duration_cast<
        std::chrono::microseconds>(utils::datetime::SteadyCoarseClock::now() - start_time_);
    pool_->AccountTransactionCommit(total_duration, busy_time_);
}

void Transaction::Rollback() {
    const utils::FastScopeGuard unlock_guard([this]() noexcept { trx_lock_.Unlock(); });
    AssertValid();
    detail::CheckDeadlineNotExpired(deadline_);
    auto connection = std::move(*connection_);
    connection->Rollback(deadline_);
    pool_->AccountTransactionRollback();
}

ResultSet Transaction::Execute(const Query& query) {
    AssertValid();
    detail::CheckDeadlineNotExpired(deadline_);
    tracing::Span span{storages::odbc::impl::tracing::kExecuteSpan};

    const auto start = utils::datetime::SteadyCoarseClock::now();
    try {
        auto result = (*connection_)->Query(query.GetStatementView(), deadline_);
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
