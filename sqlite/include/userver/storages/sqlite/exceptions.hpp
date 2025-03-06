#pragma once

/// @file userver/storages/sqlite/exceptions.hpp

#include <stdexcept>

#include <sqlite3.h>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

/// @brief Base class for all uSQLite driver exceptions
class SQLiteException : public std::runtime_error {
 public:
  SQLiteException(const char* error_message, int error_code);

  SQLiteException(const std::string& message, int error_code);

  explicit SQLiteException(const char* error_message);

  explicit SQLiteException(const std::string& message);

  explicit SQLiteException(sqlite3* sqlite_object);

  SQLiteException(sqlite3* sqlite_object, int error_code);

  ~SQLiteException() override;

  /// Return the result code (if any, otherwise -1).
  int getErrorCode() const noexcept;

  /// Return the extended numeric result code (if any, otherwise -1).
  int getExtendedErrorCode() const noexcept;

  /// Return a string, solely based on the error code
  const char* getErrorStr() const noexcept;

 private:
  int error_code_;           // Error code value
  int extended_error_code_;  // Detailed error code if any
};

/// @brief Statement exception - something went wrong with the statement
class SQLiteStatementException : public SQLiteException {
 public:
  using SQLiteException::SQLiteException;

  ~SQLiteStatementException() override;
};

/// @brief Transaction exception - something went wrong with the transaction
class SQLiteTransactionException : public SQLiteException {
 public:
  using SQLiteException::SQLiteException;

  ~SQLiteTransactionException() override;
};

// TODO: Added other SQLite exceptions

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
