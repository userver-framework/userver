#include <userver/storages/sqlite/transaction.hpp>

#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/sqlite/query.hpp>
#include "userver/storages/sqlite/exceptions.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Transaction::Transaction(sqlite3* handle,
                         engine::TaskProcessor& blocking_task_processor,
                         const TransactionOptions& options,
                         impl::StatementsCache& statements_cache)
    : handle_(handle),
      blocking_task_processor_(blocking_task_processor),
      statements_cache_(statements_cache) {
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

void Transaction::Commit() {
  if (commited_) {
    throw SQLiteException("Transaction already commited");
  }
  Execute("COMMIT TRANSACTION");
  commited_ = true;
}

void Transaction::Rollback() {
  if (commited_) {
    throw SQLiteException("Transaction already commited");
  }
  Execute("ROLLBACK TRANSACTION");
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
