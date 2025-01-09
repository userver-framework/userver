#include <userver/storages/sqlite/transaction.hpp>

#include "userver/engine/async.hpp"
#include "userver/logging/log.hpp"

#include "userver/storages/sqlite/query.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

Transaction::Transaction(sqlite3* handle,
                         engine::TaskProcessor& blocking_task_processor)
    : handle_(handle), blocking_task_processor_(blocking_task_processor) {}

Transaction::Transaction(const Transaction& other) = default;

Transaction::Transaction(Transaction&& other) noexcept = default;

Transaction::~Transaction() {
  try {
    Rollback();
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Failed to auto rollback a transaction: " << ex.what();
  }
}

void Transaction::Commit() {}

void Transaction::Rollback() {}

ResultSet Transaction::DoExecute(const Query& query) const {
  return engine::AsyncNoSpan(
             blocking_task_processor_,
             [this, query] {
               sqlite3_stmt* stmt = nullptr;
               int ret = 0;
               if (ret =
                       sqlite3_prepare_v2(handle_, query.GetStatement().c_str(),
                                          -1, &stmt, nullptr);
                   ret != SQLITE_OK) {
                 throw SQLiteException(handle_, ret);
               }
               const int exec_status = sqlite3_step(stmt);
               if (exec_status != SQLITE_ROW && exec_status != SQLITE_DONE) {
                 throw SQLiteException(handle_, exec_status);
               }
               return ResultSet(stmt, exec_status);
             })
      .Get();
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
