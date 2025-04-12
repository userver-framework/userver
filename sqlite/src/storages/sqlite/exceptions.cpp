#include <userver/storages/sqlite/exceptions.hpp>

#include <userver/storages/sqlite/impl/sqlite3_include.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

SQLiteException::SQLiteException(const char* error_message, int error_code, int extended_error_code)
    : std::runtime_error{error_message}, error_code_{error_code}, extended_error_code_{extended_error_code} {}

SQLiteException::SQLiteException(const std::string& error_message, int error_code, int extended_error_code)
    : SQLiteException(error_message.c_str(), error_code, extended_error_code) {}

SQLiteException::SQLiteException(const char* error_message, int error_code)
    : std::runtime_error{error_message}, error_code_{error_code}, extended_error_code_{-1} {}

SQLiteException::SQLiteException(const std::string& error_message, int error_code)
    : SQLiteException(error_message.c_str(), error_code) {}

SQLiteException::SQLiteException(const char* error_message) : SQLiteException{error_message, -1} {}

SQLiteException::SQLiteException(const std::string& error_message) : SQLiteException{error_message.c_str(), -1} {}

SQLiteException::~SQLiteException() = default;

int SQLiteException::getErrorCode() const noexcept { return error_code_; }

int SQLiteException::getExtendedErrorCode() const noexcept { return extended_error_code_; }

const char* SQLiteException::getErrorStr() const noexcept { return sqlite3_errstr(error_code_); }

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
