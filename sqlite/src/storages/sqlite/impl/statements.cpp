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

const std::string& Statement::GetStatementText() const { return statement_; }

std::string Statement::getExpandedStatementText() const {
  char* expanded = sqlite3_expanded_sql(prepare_statement_.get());
  std::string expandedString(expanded);
  sqlite3_free(expanded);
  return expandedString;
}

Statement::NativeStatementPtr Statement::prepareStatement() {
  sqlite3_stmt* statement = nullptr;
  const int ret = sqlite3_prepare_v2(db_handler_, statement_.c_str(),
                                     static_cast<int>(statement_.size()),
                                     &statement, nullptr);
  if (ret != SQLITE_OK) {
    throw SQLiteException(db_handler_, ret);
  }

  return Statement::NativeStatementPtr(
      statement, [](sqlite3_stmt* stmt) { sqlite3_finalize(stmt); });
}

void Statement::Reset() {
  sqlite3_reset(prepare_statement_.get());
  sqlite3_clear_bindings(prepare_statement_.get());
  has_row_ = false;
  done_ = false;
}

void Statement::Bind(const int index, const int32_t value) {
  sqlite3_bind_int(prepare_statement_.get(), index, value);
}

void Statement::Bind(const int index, const uint32_t value) {
  sqlite3_bind_int64(prepare_statement_.get(), index, value);
}

void Statement::Bind(const int index, const int64_t value) {
  sqlite3_bind_int64(prepare_statement_.get(), index, value);
}

void Statement::Bind(const int index, const double value) {
  sqlite3_bind_double(prepare_statement_.get(), index, value);
}

void Statement::Bind(const int index, const std::string& value) {
  // TODO: Which mode of destructor is better SQLITE_TRANSIENT or SQLITE_STATIC
  sqlite3_bind_text(prepare_statement_.get(), index, value.c_str(),
                    static_cast<int>(value.size()), SQLITE_TRANSIENT);
}

void Statement::Bind(const int index, const std::string_view value) {
  // TODO: Which mode of destructor is better SQLITE_TRANSIENT or SQLITE_STATIC
  sqlite3_bind_text(prepare_statement_.get(), index, std::string(value).c_str(),
                    static_cast<int>(value.size()), SQLITE_TRANSIENT);
}

void Statement::Bind(const int index, const char* value, const int size) {
  sqlite3_bind_blob(prepare_statement_.get(), index, value, size,
                    SQLITE_TRANSIENT);
}

void Statement::Bind(const int index) {
  sqlite3_bind_null(prepare_statement_.get(), index);
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
