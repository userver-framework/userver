#include <userver/storages/sqlite/impl/statement.hpp>

#include <fmt/format.h>

#include <userver/tracing/scope_time.hpp>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/impl/sqlite3_include.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

Statement::Statement(const NativeHandler& db_handler,
                     const std::string& statement)
    : db_handler_{db_handler},
      prepare_statement_(prepareStatement(statement)),
      column_count_(sqlite3_column_count(prepare_statement_.get())) {}

Statement::~Statement() = default;

Statement::Statement(Statement&& other) noexcept = default;

void Statement::SQLiteStatementDeleter::operator()(sqlite3_stmt* stmt) {
  // TODO: is this an I/O bound operation, does it need to be run on
  // blocking_task_processor_?

  // It's return last execution error status, we do not need to check it here
  sqlite3_finalize(stmt);
}

std::string Statement::GetStatementText() const noexcept {
  const char* query = sqlite3_sql(prepare_statement_.get());
  if (!query) {
    return std::string{};
  }
  std::string queryString{query};
  return queryString;
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

Statement::NativeStatementPtr Statement::prepareStatement(
    const std::string& statement_str) {
  sqlite3_stmt* statement = nullptr;
  // TODO: It can indirectly triggers I/O?
  const int ret_code = sqlite3_prepare_v2(
      db_handler_.GetHandle(), statement_str.c_str(),
      static_cast<int>(statement_str.size()), &statement, nullptr);
  if (ret_code != SQLITE_OK) {
    throw SQLiteException(sqlite3_errmsg(db_handler_.GetHandle()), ret_code,
                          sqlite3_extended_errcode(db_handler_.GetHandle()));
  }

  return Statement::NativeStatementPtr(statement, SQLiteStatementDeleter());
}

int32_t Statement::GetInt32Column(int column) const noexcept {
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
  auto text_ptr = reinterpret_cast<const char*>(
      sqlite3_column_text(prepare_statement_.get(), column));
  return text_ptr ? text_ptr : "";
}

const void* Statement::GetBlobColumn(int column) const noexcept {
  return sqlite3_column_blob(prepare_statement_.get(), column);
}

std::string Statement::GetStringColumn(int column) const noexcept {
  auto data = static_cast<const char*>(GetBlobColumn(column));
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
    throw SQLiteException(sqlite3_errmsg(db_handler_.GetHandle()), exec_status_,
                          sqlite3_extended_errcode(db_handler_.GetHandle()));
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

void Statement::CheckCode(const int ret_code) const {
  if (ret_code != SQLITE_OK) {
    throw SQLiteException(sqlite3_errmsg(db_handler_.GetHandle()), ret_code,
                          sqlite3_extended_errcode(db_handler_.GetHandle()));
  }
}

void Statement::Bind(const int index, const int32_t value) {
  const int ret_code = sqlite3_bind_int(prepare_statement_.get(), index, value);
  CheckCode(ret_code);
}

void Statement::Bind(const int index, const int64_t value) {
  const int ret_code =
      sqlite3_bind_int64(prepare_statement_.get(), index, value);
  CheckCode(ret_code);
}

void Statement::Bind(const int index, const uint32_t value) {
  const int ret_code =
      sqlite3_bind_int64(prepare_statement_.get(), index, value);
  CheckCode(ret_code);
}

void Statement::Bind(const int index, const uint64_t value) {
  const int ret_code =
      sqlite3_bind_int64(prepare_statement_.get(), index, value);
  CheckCode(ret_code);
}

void Statement::Bind(const int index, const double value) {
  const int ret_code =
      sqlite3_bind_double(prepare_statement_.get(), index, value);
  CheckCode(ret_code);
}

void Statement::Bind(const int index, const std::string& value) {
  const int ret_code =
      sqlite3_bind_text(prepare_statement_.get(), index, value.c_str(),
                        static_cast<int>(value.size()), SQLITE_TRANSIENT);
  CheckCode(ret_code);
}

void Statement::Bind(const int index, const std::string_view value) {
  Bind(index, std::string(value));
}

void Statement::Bind(const int index, const char* value, const int size) {
  const int ret_code = sqlite3_bind_blob(prepare_statement_.get(), index, value,
                                         size, SQLITE_TRANSIENT);
  CheckCode(ret_code);
}

void Statement::Bind(const int index) {
  const int ret_code = sqlite3_bind_null(prepare_statement_.get(), index);
  CheckCode(ret_code);
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
