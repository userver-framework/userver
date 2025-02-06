#include <userver/storages/sqlite/result_set.hpp>

#include <userver/storages/sqlite/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

ResultSet::ResultSet(std::shared_ptr<sqlite3_stmt> stmt, int exec_status)
    : stmt_(std::move(stmt)), exec_status_(exec_status) {
  if (!stmt_) throw SQLiteException("Statement cannot be null");
}

ResultSet::ResultSet(ResultSet&& other) noexcept = default;

ResultSet& ResultSet::operator=(ResultSet&&) noexcept = default;

ResultSet::~ResultSet() = default;

ExecutionResult ResultSet::AsExecutionResult() && {
  // TODO: need check on SQLITE_DONE?
  // TODO: is this an CPU bound operation? does it need to be run on
  // blocking_task_processor_?
  const auto rows_affected = sqlite3_changes(sqlite3_db_handle(stmt_.get()));
  const auto last_insert_id =
      sqlite3_last_insert_rowid(sqlite3_db_handle(stmt_.get()));

  ExecutionResult result{};
  result.rows_affected = rows_affected;
  result.last_insert_id = last_insert_id;
  return result;
}

// TODO: Add support of NULL and NULLABLE types?

template <>
int32_t ResultSet::GetColumn<int32_t>(sqlite3_stmt* stmt, int column) {
  // TODO: is this an CPU bound operation? does it need to be run on
  // blocking_task_processor_?
  return sqlite3_column_int(stmt, column);
}

template <>
uint32_t ResultSet::GetColumn<uint32_t>(sqlite3_stmt* stmt, int column) {
  // TODO: Check for null, what to return if the value is null, not 0
  return sqlite3_column_int64(stmt, column);
}

template <>
int64_t ResultSet::GetColumn<int64_t>(sqlite3_stmt* stmt, int column) {
  return sqlite3_column_int64(stmt, column);
}

template <>
double ResultSet::GetColumn<double>(sqlite3_stmt* stmt, int column) {
  return sqlite3_column_double(stmt, column);
}

template <>
const char* ResultSet::GetColumn<const char*>(sqlite3_stmt* stmt, int column) {
  // Return a pointer to the text value (NULL terminated string) of the column
  // specified by its index starting at 0
  auto text_ptr =
      reinterpret_cast<const char*>(sqlite3_column_text(stmt, column));
  return text_ptr ? text_ptr : "";
}

template <>
std::string ResultSet::GetColumn<std::string>(sqlite3_stmt* stmt, int column) {
  // Note: using sqlite3_column_blob and not sqlite3_column_text
  // - no need for sqlite3_column_text to add a \0 on the end, as we're getting
  // the bytes length directly
  //   however, we need to call sqlite3_column_bytes() to ensure correct format.
  //   It's a noop on a BLOB or a TEXT value with the correct encoding (UTF-8).
  //   Otherwise it'll do a conversion to TEXT (UTF-8).
  // (void)sqlite3_column_bytes(stmt, column);
  auto data = static_cast<const char*>(sqlite3_column_blob(stmt, column));
  // SQLite docs: "The safest policy is to invoke… sqlite3_column_blob()
  // followed by sqlite3_column_bytes()"
  // Note: std::string is ok to pass nullptr as first arg, if length is 0
  return std::string(data, sqlite3_column_bytes(stmt, column));
}

template <>
const void* ResultSet::GetColumn<const void*>(sqlite3_stmt* stmt, int column) {
  return sqlite3_column_blob(stmt, column);
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
