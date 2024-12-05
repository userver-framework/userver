#pragma once

/// @file userver/storages/sqlite/exceptions.hpp

#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

/// @brief Base class for all uSQLite driver exceptions
class SQLiteException : public std::runtime_error {
 public:
  SQLiteException(unsigned int error, const char* message);
  SQLiteException(unsigned int error, const std::string& message);

  ~SQLiteException() override;

  unsigned int GetErrno() const;

 private:
  unsigned int errno_;
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

// TODO: Added SQLite exceptions

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
