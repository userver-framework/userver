#pragma once

#include <stdexcept>

namespace storages {
namespace postgres {

//@{
/** @name Generic driver errors */
class PostgresError : public std::runtime_error {
  using runtime_error::runtime_error;
};
//@}

//@{
/** @name Connection errors */

class ConnectionError : public PostgresError {
  using PostgresError::PostgresError;
  // TODO Add connection info
};

/// @brief Exception is thrown when a single connection fails to connect
class ConnectionFailed : public ConnectionError {
 public:
  ConnectionFailed()
      : ConnectionError("Failed to connect to PostgreSQL server") {}
};

/// @brief Indicates errors during pool operation
class PoolError : public ConnectionError {
  using ConnectionError::ConnectionError;
};

class ClusterUnavailable : public ConnectionError {
  using ConnectionError::ConnectionError;
};
//@}

//@{
class QueryError : public PostgresError {
  using PostgresError::PostgresError;
};

class ConstraintViolation : public QueryError {};

//@}

//@{
/** @name Transaction errors */
class TransactionError : public PostgresError {
  using PostgresError::PostgresError;
};

class AlreadyInTransaction : public TransactionError {
 public:
  AlreadyInTransaction()
      : TransactionError("Connection is already in transaction block") {}
};

class NotInTransaction : public TransactionError {
 public:
  NotInTransaction()
      : TransactionError("Connection is not in transaction block") {}
  NotInTransaction(const char* msg) : TransactionError(msg) {}
};
//@}

//@{
/** @name Misc exceptions */
// TODO Fit exceptions into a hierarchy
class RowIndexOutOfBounds : public PostgresError {
 public:
  RowIndexOutOfBounds(std::size_t /*index*/)
      : PostgresError("Row index is out of bounds") {}
};
class FieldIndexOutOfBounds : public PostgresError {
 public:
  FieldIndexOutOfBounds(std::size_t /*index*/)
      : PostgresError("Field index is out of bounds") {}
};

class FieldNameDoesntExist : public PostgresError {
 public:
  FieldNameDoesntExist(const std::string& name)
      : PostgresError("Field name '" + name + "' doesn't exist") {}
};

class InvalidDSN : public PostgresError {
 public:
  InvalidDSN(const std::string& dsn, const char* err)
      : PostgresError("Invalid dsn '" + dsn + "': " + err) {}
};

class InvalidConfig : public PostgresError {
  using PostgresError::PostgresError;
};

//@}

}  // namespace postgres
}  // namespace storages
