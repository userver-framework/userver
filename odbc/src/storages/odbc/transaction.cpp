#include <userver/storages/odbc/transaction.hpp>

#include <storages/odbc/detail/connection.hpp>
#include <storages/odbc/detail/conn_ptr.hpp>
#include <userver/storages/odbc/impl/tracing_tags.hpp>

#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

Transaction::Transaction(detail::ConnectionPtr&& connection)
    : connection_{std::move(connection)},
      span_{storages::odbc::impl::tracing::kTransactionSpan}
{
    (*connection_)->Begin();
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
    AssertValid();
    {
        auto connection = std::move(connection_);
        (*connection)->Commit();
    }
    trx_lock_.Unlock();
}

void Transaction::Rollback() {
    AssertValid();
    {
        auto connection = std::move(connection_);
        (*connection)->Rollback();
    }
    trx_lock_.Unlock();
}

ResultSet Transaction::Execute(const Query& query) const {
    AssertValid();
    tracing::Span span{storages::odbc::impl::tracing::kExecuteSpan};
    return (*connection_)->Query(query.GetStatementView());
}

void Transaction::AssertValid() const {
    UINVARIANT(connection_->IsValid(), "Transaction accessed after it's been committed (or rolled back)");
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
