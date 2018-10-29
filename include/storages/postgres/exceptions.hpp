#pragma once

#include <stdexcept>

namespace storages {
namespace postgres {

//@{
/** @name Connection errors */

class ConnectionError : public std::runtime_error {
 public:
  ConnectionError(const std::string& msg) : runtime_error(msg) {}
  ConnectionError(const char* msg) : runtime_error(msg) {}
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
class QueryError : public std::runtime_error {
 public:
  QueryError(const std::string& msg) : runtime_error(msg) {}
  QueryError(const char* msg) : runtime_error(msg) {}
};

class ConstraintViolation : public QueryError {};

//@}

//@{
/** @name Transaction errors */
class TransactionError : public std::runtime_error {
 public:
  TransactionError(const std::string msg) : runtime_error(msg) {}
  TransactionError(const char* msg) : runtime_error(msg) {}
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
class RowIndexOutOfBounds : public std::runtime_error {
 public:
  RowIndexOutOfBounds(std::size_t /*index*/)
      : runtime_error("Row index is out of bounds") {}
};
class FieldIndexOutOfBounds : public std::runtime_error {
 public:
  FieldIndexOutOfBounds(std::size_t /*index*/)
      : runtime_error("Field index is out of bounds") {}
};

class FieldNameDoesntExist : public std::runtime_error {
 public:
  FieldNameDoesntExist(const std::string& name)
      : runtime_error("Field name '" + name + "' doesn't exist") {}
};

class InvalidDSN : public std::runtime_error {
 public:
  InvalidDSN(const std::string& dsn, const char* err)
      : runtime_error("Invalid dsn '" + dsn + "': " + err) {}
};

class InvalidConfig : public std::logic_error {
  using logic_error::logic_error;
};

//@}

}  // namespace postgres
}  // namespace storages
