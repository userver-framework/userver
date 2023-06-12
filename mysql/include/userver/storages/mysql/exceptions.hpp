#pragma once

/// @file userver/storages/mysql/exceptions.hpp

#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

/// @brief Base class for all uMySQL driver exceptions
class MySQLException : public std::runtime_error {
 public:
  MySQLException(unsigned int error, const char* message);
  MySQLException(unsigned int error, const std::string& message);

  ~MySQLException() override;

  unsigned int GetErrno() const;

 private:
  unsigned int errno_;
};

/// @brief IO exception (read/write timeout/cancelled)
class MySQLIOException : public MySQLException {
 public:
  using MySQLException::MySQLException;

  ~MySQLIOException() override;
};

/// @brief Statement exception - something went wrong with the statement
class MySQLStatementException : public MySQLException {
 public:
  using MySQLException::MySQLException;

  ~MySQLStatementException() override;
};

/// @brief Command exception - something went wrong with the command
class MySQLCommandException : public MySQLException {
 public:
  using MySQLException::MySQLException;

  ~MySQLCommandException() override;
};

/// @brief Transaction exception - something went wrong with the transaction
class MySQLTransactionException : public MySQLException {
 public:
  using MySQLException::MySQLException;

  ~MySQLTransactionException() override;
};

/// @brief
class MySQLValidationException : public MySQLException {
 public:
  using MySQLException::MySQLException;

  ~MySQLValidationException() override;
};

}  // namespace storages::mysql

USERVER_NAMESPACE_END
