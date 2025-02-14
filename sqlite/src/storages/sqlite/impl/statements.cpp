#include <userver/storages/sqlite/impl/statements.hpp>

#include <fmt/format.h>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/tracing/scope_time.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

Statement::Statement(sqlite3* db_handler, const std::string& statement)
    : db_handler_{db_handler},
      statement_{statement},
      prepare_statement_(prepareStatement()),
      column_count_(sqlite3_column_count(prepare_statement_.get())) {}

Statement::~Statement() = default;

Statement::Statement(Statement&& other) noexcept = default;

void Statement::SQLiteStatementDeleter::operator()(sqlite3_stmt* stmt) {
  // TODO: is this an I/O bound operation, does it need to be run on
  // blocking_task_processor_?

  // It's return last execution error status, we do not need to check it here
  sqlite3_finalize(stmt);
}

const std::string& Statement::GetStatementText() const noexcept {
  return statement_;
}

std::string Statement::getExpandedStatementText() const noexcept {
  char* expanded = sqlite3_expanded_sql(prepare_statement_.get());
  if (!expanded) {
    return std::string{};
  }
  std::string expandedString{expanded};
  sqlite3_free(expanded);
  return expandedString;
}

Statement::NativeStatementPtr Statement::prepareStatement() {
  sqlite3_stmt* statement = nullptr;
  const int ret_code = sqlite3_prepare_v2(db_handler_, statement_.c_str(),
                                          static_cast<int>(statement_.size()),
                                          &statement, nullptr);
  if (ret_code != SQLITE_OK) {
    throw SQLiteException(db_handler_, ret_code);
  }

  return Statement::NativeStatementPtr(statement, SQLiteStatementDeleter());
}

int32_t Statement::GetInt32Column(int column) const noexcept {
  // TODO: is this an CPU bound operation? does it need to be run on
  // blocking_task_processor_?
  // TODO: Check for null, what to return if the value is null, not 0
  return sqlite3_column_int(prepare_statement_.get(), column);
}

uint32_t Statement::GetUInt32Column(int column) const noexcept {
  return GetInt64Column(column);
}

int64_t Statement::GetInt64Column(int column) const noexcept {
  return sqlite3_column_int64(prepare_statement_.get(), column);
}

double Statement::GetDoubleColumn(int column) const noexcept {
  return sqlite3_column_double(prepare_statement_.get(), column);
}

const char* Statement::GetCStringColumn(int column) const noexcept {
  // Return a pointer to the text value (NULL terminated string) of the column
  // specified by its index starting at 0
  auto text_ptr = reinterpret_cast<const char*>(
      sqlite3_column_text(prepare_statement_.get(), column));
  return text_ptr ? text_ptr : "";
}

const void* Statement::GetBlobColumn(int column) const noexcept {
  return sqlite3_column_blob(prepare_statement_.get(), column);
}

std::string Statement::GetStringColumn(int column) const noexcept {
  // Note: using sqlite3_column_blob and not sqlite3_column_text
  // - no need for sqlite3_column_text to add a \0 on the end, as we're getting
  // the bytes length directly
  //   however, we need to call sqlite3_column_bytes() to ensure correct format.
  //   It's a noop on a BLOB or a TEXT value with the correct encoding (UTF-8).
  //   Otherwise it'll do a conversion to TEXT (UTF-8).
  // (void)sqlite3_column_bytes(stmt, column);
  auto data = static_cast<const char*>(GetBlobColumn(column));
  // SQLite docs: "The safest policy is to invoke… sqlite3_column_blob()
  // followed by sqlite3_column_bytes()"
  // Note: std::string is ok to pass nullptr as first arg, if length is 0
  return std::string(data,
                     sqlite3_column_bytes(prepare_statement_.get(), column));
}

std::vector<uint8_t> Statement::GetBytesColumn(int column) const noexcept {
  const void* blob = GetBlobColumn(column);
  int size = sqlite3_column_bytes(prepare_statement_.get(), column);
  return blob ? std::vector<uint8_t>(static_cast<const uint8_t*>(blob),
                                     static_cast<const uint8_t*>(blob) + size)
              : std::vector<uint8_t>{};
}

int Statement::RowsAffected() const noexcept {
  // TODO: need check on SQLITE_DONE?
  // TODO: is this an CPU bound operation? does it need to be run on
  // blocking_task_processor_?
  return sqlite3_changes(sqlite3_db_handle(prepare_statement_.get()));
}

int Statement::LastInsertRowId() const noexcept {
  return sqlite3_last_insert_rowid(sqlite3_db_handle(prepare_statement_.get()));
}

bool Statement::HasNext() const noexcept { return exec_status_ == SQLITE_ROW; }

bool Statement::IsDone() const noexcept { return exec_status_ == SQLITE_DONE; }

bool Statement::IsFail() const noexcept { return !IsDone() && !HasNext(); }

void Statement::CheckFail() const {
  if (IsFail()) {
    throw SQLiteException(db_handler_, exec_status_);
  }
}

void Statement::Next() noexcept {
  exec_status_ = sqlite3_step(prepare_statement_.get());
  if (IsDone()) {
    Reset();
  }
}

int Statement::ColumnCount() const noexcept {
  return sqlite3_column_count(prepare_statement_.get());
}

void Statement::Reset() noexcept {
  // It's return last execution error status, we do not need to check it here
  sqlite3_reset(prepare_statement_.get());
  sqlite3_clear_bindings(
      prepare_statement_.get());  // reset all host parameters to NULL
}

void Statement::Bind(const int index, const int32_t value) {
  const int ret_code = sqlite3_bind_int(prepare_statement_.get(), index, value);
  if (ret_code != SQLITE_OK) {
    throw SQLiteException(db_handler_, ret_code);
  }
}

void Statement::Bind(const int index, const int64_t value) {
  const int ret_code =
      sqlite3_bind_int64(prepare_statement_.get(), index, value);
  if (ret_code != SQLITE_OK) {
    throw SQLiteException(db_handler_, ret_code);
  }
}

void Statement::Bind(const int index, const uint32_t value) {
  const int ret_code =
      sqlite3_bind_int64(prepare_statement_.get(), index, value);
  if (ret_code != SQLITE_OK) {
    throw SQLiteException(db_handler_, ret_code);
  }
}

void Statement::Bind(const int index, const uint64_t value) {
  const int ret_code =
      sqlite3_bind_int64(prepare_statement_.get(), index, value);
  if (ret_code != SQLITE_OK) {
    throw SQLiteException(db_handler_, ret_code);
  }
}

void Statement::Bind(const int index, const double value) {
  const int ret_code =
      sqlite3_bind_double(prepare_statement_.get(), index, value);
  if (ret_code != SQLITE_OK) {
    throw SQLiteException(db_handler_, ret_code);
  }
}

void Statement::Bind(const int index, const std::string& value) {
  // TODO: Which mode of destructor is better SQLITE_TRANSIENT or SQLITE_STATIC
  const int ret_code =
      sqlite3_bind_text(prepare_statement_.get(), index, value.c_str(),
                        static_cast<int>(value.size()), SQLITE_TRANSIENT);
  if (ret_code != SQLITE_OK) {
    throw SQLiteException(db_handler_, ret_code);
  }
}

void Statement::Bind(const int index, const std::string_view value) {
  Bind(index, std::string(value));
}

void Statement::Bind(const int index, const char* value, const int size) {
  // TODO: Which mode of destructor is better SQLITE_TRANSIENT or SQLITE_STATIC
  const int ret_code = sqlite3_bind_blob(prepare_statement_.get(), index, value,
                                         size, SQLITE_TRANSIENT);
  if (ret_code != SQLITE_OK) {
    throw SQLiteException(db_handler_, ret_code);
  }
}

void Statement::Bind(const int index) {
  const int ret_code = sqlite3_bind_null(prepare_statement_.get(), index);
  if (ret_code != SQLITE_OK) {
    throw SQLiteException(db_handler_, ret_code);
  }
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
