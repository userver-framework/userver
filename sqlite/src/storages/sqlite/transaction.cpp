#include <userver/storages/sqlite/transaction.hpp>

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/sqlite/query.hpp>
#include "userver/storages/sqlite/exceptions.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Transaction::Transaction(std::shared_ptr<impl::ConnectionImpl> pimpl,
                         const TransactionOptions& options)
    : pimpl_(std::move(pimpl)) {
  switch (options.mode) {
    case TransactionOptions::kDeferred:
      Execute("BEGIN DEFERRED");
      break;
    case TransactionOptions::kImmediate:
      Execute("BEGIN IMMEDIATE");
      break;
    case TransactionOptions::kExclusive:
      Execute("BEGIN EXCLUSIVE");
      break;
    default:
      throw SQLiteException("invalid/unknown transaction mode");
  }
}

Transaction::Transaction(const Transaction& other) = default;

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
