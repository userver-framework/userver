#include <userver/storages/sqlite/result_set.hpp>

#include <userver/storages/sqlite/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

ResultSet::ResultSet(sqlite3_stmt* stmt, int exec_status)
    : stmt_(stmt), exec_status_(exec_status) {
  if (!stmt_) throw SQLiteException("Statement cannot be null");
}

ResultSet::ResultSet(ResultSet&& other) noexcept = default;

ResultSet::~ResultSet() = default;

void ResultSet::Deleter::operator()(sqlite3_stmt* stmt) {
  // TODO: is this an CPU bound operation? does it need to be run on
  // blocking_task_processor_?
  sqlite3_finalize(stmt);
}

ExecutionResult ResultSet::AsExecutionResult() && {
  // TODO: need check on SQLITE_DONE?
  // TODO: is this an CPU bound operation? does it need to be run on
  // blocking_task_processor_?
  const auto rows_affected = sqlite3_changes(sqlite3_db_handle(stmt_.get()));
  const auto last_insert_id =
      sqlite3_last_insert_rowid(sqlite3_db_handle(stmt_.get()));

  // Reset state to use prepared statement again
  sqlite3_reset(stmt_.get());

  ExecutionResult result{};
  result.rows_affected = rows_affected;
  result.last_insert_id = last_insert_id;
  return result;
}

// Add support of NULL and NULLABLE types?
template <>
int64_t ResultSet::GetColumn<int64_t>(sqlite3_stmt* stmt, int column) {
  // TODO: is this an CPU bound operation? does it need to be run on
  // blocking_task_processor_?
  return sqlite3_column_int64(stmt, column);
}

template <>
double ResultSet::GetColumn<double>(sqlite3_stmt* stmt, int column) {
  return sqlite3_column_double(stmt, column);
}

template <>
std::string ResultSet::GetColumn<std::string>(sqlite3_stmt* stmt, int column) {
  const char* text =
      reinterpret_cast<const char*>(sqlite3_column_text(stmt, column));
  return text ? text : "";
}

template <>
std::vector<uint8_t> ResultSet::GetColumn<std::vector<uint8_t>>(
    sqlite3_stmt* stmt, int column) {
  const void* blob = sqlite3_column_blob(stmt, column);
  int size = sqlite3_column_bytes(stmt, column);
  return blob ? std::vector<uint8_t>(static_cast<const uint8_t*>(blob),
                                     static_cast<const uint8_t*>(blob) + size)
              : std::vector<uint8_t>{};
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
