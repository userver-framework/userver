#include <userver/storages/sqlite/impl/result_wrapper.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

ResultWrapper::ResultWrapper(std::shared_ptr<sqlite3_stmt> stmt,
                             int exec_status)
    : stmt_(std::move(stmt)), exec_status_(exec_status) {
  UASSERT(stmt_);
}

ResultWrapper::~ResultWrapper() = default;

int ResultWrapper::RowsAffected() const noexcept {
  // TODO: need check on SQLITE_DONE?
  // TODO: is this an CPU bound operation? does it need to be run on
  // blocking_task_processor_?
  return sqlite3_changes(sqlite3_db_handle(stmt_.get()));
}

int ResultWrapper::LastInsertRowId() const noexcept {
  return sqlite3_last_insert_rowid(sqlite3_db_handle(stmt_.get()));
}

bool ResultWrapper::HasNext() const noexcept {
  return exec_status_ == SQLITE_ROW;
}

bool ResultWrapper::IsDone() const noexcept {
  return exec_status_ == SQLITE_DONE;
}

void ResultWrapper::Next() noexcept {
  exec_status_ = sqlite3_step(stmt_.get());
  if (IsDone()) {
    sqlite3_reset(stmt_.get());
  }
}

int ResultWrapper::ColumnCount() const noexcept {
  return sqlite3_column_count(stmt_.get());
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
