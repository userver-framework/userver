#include <userver/storages/odbc/transaction.hpp>

#include <storages/odbc/detail/connection.hpp>
#include <storages/odbc/detail/conn_ptr.hpp>
#include <storages/odbc/detail/deadline.hpp>
#include <userver/storages/odbc/impl/tracing_tags.hpp>

#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/fast_scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

Transaction::Transaction(detail::ConnectionPtr&& connection, engine::Deadline deadline)
    : connection_{std::move(connection)},
      deadline_{deadline},
      span_{storages::odbc::impl::tracing::kTransactionSpan}
{
    detail::CheckDeadlineNotExpired(deadline_);
    (*connection_)->Begin(deadline_);
    trx_lock_.Lock();
}

Transaction::Transaction(Transaction&& other) noexcept = default;

Transaction::~Transaction() {
    if (connection_->IsValid()) {
        try {
            Rollback();
        } catch (const std::exception& ex) {
            LOG_ERROR() << "Failed to auto rollback a transaction: " << ex.what();
        }
    }
}

void Transaction::Commit() {
    const utils::FastScopeGuard unlock_guard([this]() noexcept { trx_lock_.Unlock(); });
    AssertValid();
    detail::CheckDeadlineNotExpired(deadline_);
    auto connection = std::move(connection_);
    (*connection)->Commit(deadline_);
}

void Transaction::Rollback() {
    const utils::FastScopeGuard unlock_guard([this]() noexcept { trx_lock_.Unlock(); });
    AssertValid();
    detail::CheckDeadlineNotExpired(deadline_);
    auto connection = std::move(connection_);
    (*connection)->Rollback(deadline_);
}

ResultSet Transaction::Execute(const Query& query) const {
    AssertValid();
    detail::CheckDeadlineNotExpired(deadline_);
    tracing::Span span{storages::odbc::impl::tracing::kExecuteSpan};
    return (*connection_)->Query(query.GetStatementView(), deadline_);
}

void Transaction::AssertValid() const {
    UINVARIANT(connection_->IsValid(), "Transaction accessed after it's been committed (or rolled back)");
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
