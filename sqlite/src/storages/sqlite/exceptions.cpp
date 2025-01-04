#include <stdexcept>

#include <userver/storages/sqlite/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

SQLiteException::SQLiteException(const char* error_message, int error_code)
    : std::runtime_error{error_message},
      error_code_{error_code},
      extended_error_code_{-1} {}

SQLiteException::SQLiteException(const std::string& error_message,
                                 int error_code)
    : SQLiteException(error_message.c_str(), error_code) {}

SQLiteException::SQLiteException(const char* error_message)
    : SQLiteException(error_message, -1) {}

SQLiteException::SQLiteException(const std::string& error_message)
    : SQLiteException(error_message.c_str(), -1) {}

SQLiteException::SQLiteException(sqlite3* sqlite_object)
    : std::runtime_error{sqlite3_errmsg(sqlite_object)},
      error_code_{sqlite3_errcode(sqlite_object)},
      extended_error_code_{sqlite3_extended_errcode(sqlite_object)} {}

SQLiteException::SQLiteException(sqlite3* sqlite_object, int error_code)
    : std::runtime_error{sqlite3_errmsg(sqlite_object)},
      error_code_{error_code},
      extended_error_code_{sqlite3_extended_errcode(sqlite_object)} {}

SQLiteException::~SQLiteException() = default;

int SQLiteException::getErrorCode() const noexcept { return error_code_; };

int SQLiteException::getExtendedErrorCode() const noexcept {
  return extended_error_code_;
};

const char* SQLiteException::getErrorStr() const noexcept {
  return sqlite3_errstr(error_code_);
};

SQLiteStatementException::~SQLiteStatementException() = default;

SQLiteTransactionException::~SQLiteTransactionException() = default;

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
