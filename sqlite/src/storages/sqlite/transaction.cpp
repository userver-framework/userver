#include <userver/storages/sqlite/transaction.hpp>

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/sqlite/query.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Transaction::Transaction(std::shared_ptr<impl::ConnectionImpl> pimpl,
                         const TransactionOptions& options)
    : pimpl_(std::move(pimpl)) {
  pimpl_->Begin(options);
}

Transaction::Transaction(Transaction&& other) noexcept = default;

Transaction::~Transaction() {
  try {
    Rollback();
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Failed to auto rollback a transaction: " << ex.what();
  }
}

void Transaction::Commit() { pimpl_->Commit(); }

void Transaction::Rollback() { pimpl_->Rollback(); }

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
