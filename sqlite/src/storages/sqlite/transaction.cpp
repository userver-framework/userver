#include <userver/storages/sqlite/transaction.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <userver/storages/sqlite/impl/connection.hpp>
#include <userver/storages/sqlite/infra/connection_ptr.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Transaction::Transaction(std::shared_ptr<infra::ConnectionPtr> connection, const settings::TransactionOptions& options)
    : connection_{std::move(connection)}
{
    if (connection_ && connection_->IsValid()) {
        (*connection_)->Begin(options);
        trx_lock_.Lock();
    }
}

Transaction::Transaction(Transaction&& other) noexcept = default;
Transaction& Transaction::operator=(Transaction&&) noexcept = default;

Transaction::~Transaction() {
    if (connection_ && connection_->IsValid()) {
        LOG_INFO()
            << "Transaction handle is destroyed without an explicit "
               "commit or rollback, rolling back automatically";
        RollbackNoThrow();
    }
}

void Transaction::AssertValid() const {
    // TODO: exception or abort?
    UINVARIANT(connection_ && connection_->IsValid(), "Transaction accessed after it's been committed");
}

Savepoint Transaction::Save(std::string name) const {
    AssertValid();
    return Savepoint{connection_, std::move(name)};
}

void Transaction::Commit() {
    AssertValid();
    utils::FastScopeGuard rollback_on_error([this]() noexcept { RollbackNoThrow(); });
    (*connection_)->Commit();
    rollback_on_error.Release();
    trx_lock_.Unlock();
    connection_.reset();
}

void Transaction::Rollback() {
    AssertValid();
    auto connection = std::move(connection_);
    const utils::FastScopeGuard unlock([this]() noexcept { trx_lock_.Unlock(); });
    try {
        (*connection)->Rollback();
    } catch (...) {
        (*connection)->NotifyBroken();
        throw;
    }
}

void Transaction::RollbackNoThrow() noexcept {
    try {
        Rollback();
    } catch (const std::exception& err) {
        LOG_ERROR() << "Failed to automatically roll back a transaction: " << err;
    }
}

ResultSet Transaction::DoExecute(impl::io::ParamsBinderBase& params) const {
    auto prepare_statement = params.GetBindsPtr();
    auto result_wrapper = std::make_unique<impl::ResultWrapper>(prepare_statement, connection_);
    return ResultSet{std::move(result_wrapper)};
}

void Transaction::AccountQueryExecute() const noexcept { (*connection_)->AccountQueryExecute(); }

void Transaction::AccountQueryFailed() const noexcept { (*connection_)->AccountQueryFailed(); }

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
