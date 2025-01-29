#include <userver/storages/sqlite/impl/statements.hpp>

#include <fmt/format.h>

#include <userver/logging/log.hpp>
#include <userver/tracing/scope_time.hpp>
#include "userver/storages/sqlite/exceptions.hpp"

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

  return Statement::NativeStatementPtr(statement);
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

void Statement::Bind(const int index, const uint32_t value) {
  const int ret_code =
      sqlite3_bind_int64(prepare_statement_.get(), index, value);
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
