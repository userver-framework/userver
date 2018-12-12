#pragma once

#include <stdexcept>

#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/message.hpp>

#include <utils/demangle.hpp>

namespace storages {
namespace postgres {

/**
 * @page Postgres errors.
 *
 * Base class for all PostgreSQL errors is Error which is derived from
 * std::runtime_error. This is done to simplify exception handling.
 *
 * There are two base types of errors: runtime (@see RuntimeError) and logic
 * (@see LogicError).
 *
 * Logic errors are a consequence of faulty logic within the program such as
 * violating logical preconditions or invariants and may be preventable by
 * correcting the program.
 *
 * Runtime errors are due to events beyond the scope of the program, such as
 * network failure, faulty configuration file, unique index violation etc. A
 * user can catch such an error and recover from it by reconnecting, providing
 * a decent default for configuration or modifying the key value.
 *
 * Both logic and runtime errors can contain a postgres server message
 * (@see Message). Those are ServerLogicError and ServerRuntimeError
 * respectively. These errors occur on the server side and are translated into
 * exceptions by the driver. Server errors are descendants of either
 * ServerLogicError or ServerRuntimeError and their hierarchy corresponds to SQL
 * error classes.
 *
 * Some server errors, such as IntegrityConstraintViolation, have a more
 * detailed hierarchy to distinguish errors in catch clauses.
 *
 * Server errors have the following hierarchy:
 *   - ServerLogicError
 *     - SqlStatementNotYetComplete
 *     - FeatureNotSupported
 *     - InvalidRoleSpecification
 *     - CardinalityViolation
 *     - InvalidObjectName
 *     - InvalidAuthorizationSpecification
 *     - SyntaxError
 *     - AccessRuleViolation (TODO Decide if this is actually a runtime error)
 *   - ServerRuntimeError
 *     - TriggeredActionException
 *     - LocatorException
 *     - InvalidGrantor
 *     - DiagnosticsException
 *     - DataException
 *     - IntegrityConstraintViolation
 *       - RestrictViolation
 *       - NotNullViolation (TODO Make it a logic error)
 *       - ForeignKeyViolation
 *       - UniqueViolation
 *       - CheckViolation
 *       - ExclusionViolation
 *       - TriggeredDataChangeViolation
 *       - WithCheckOptionViolation
 *     - InvalidCursorState
 *     - InvalidTransactionState
 *     - DependentPrivilegeDescriptorsStillExist
 *     - InvalidTransactionTermination
 *     - ExternalRoutineException
 *     - ExternalRoutineInvocationException
 *     - SavepointException
 *     - SqlRoutineException
 *     - TransactionRollback
 *     - InsufficientResources
 *     - ProgramLimitExceeded
 *     - ObjectNotInPrerequisiteState
 *     - OperatorIntervention
 *       - QueryCanceled
 *       - AdminShutdown
 *       - CrashShutdown
 *       - CannotConnectNow
 *       - DatabaseDropped
 *     - SystemError
 *     - SnapshotFailure
 *     - ConfigurationFileError
 *     - FdwError
 *     - PlPgSqlError
 *     - InternalServerError
 *
 * Besides server errors there are exceptions thrown by the driver itself,
 * those are:
 *   - LogicError
 *     - ResultSetError
 *       - FieldIndexOutOfBounds
 *       - FieldNameDoenstExist
 *       - FieldTupleMismatch
 *       - FieldValueIsNull
 *       - InvalidInputBufferSize
 *       - InvalidTupleSizeRequested
 *       - NoValueParser
 *       - RowIndexOutOfBounds
 *       - TextParseFailure
 *       - TypeCannotBeNull
 *     - ArrayError
 *       - DimensionMismatch
 *       - InvalidDimensions
 *     - EnumerationError
 *       - InvalidEnumerationLiteral
 *       - InvalidEnumerationValue
 *     - TransactionError
 *       - AlreadyInTransaction
 *       - NotInTransaction
 *   - RuntimeError
 *     - ConnectionError
 *       - ClusterUnavailable
 *       - CommandError
 *       - ConnectionFailed
 *       - PoolError
 *       - ServerConnectionError (contains a message from server)
 *     - InvalidConfig
 *     - InvalidDSN
 */

//@{
/** @name Generic driver errors */

/// @brief Base class for all exceptions that may be thrown by the driver.
class Error : public std::runtime_error {
  using runtime_error::runtime_error;
};

/// @brief Base Postgres logic error.
/// Reports errors that are consequences of erroneous driver usage,
/// such as invalid query syntax, absence of appropriate parsers, out of range
/// errors etc.
/// These can be avoided by fixing code.
class LogicError : public Error {
  using Error::Error;
};

/// @brief Base Postgres runtime error.
/// Reports errors that are consequences of erroneous data, misconfiguration,
/// network errors etc.
class RuntimeError : public Error {
  using Error::Error;
};

/// @brief Error that was reported by PosgtreSQL server
/// Contains the message sent by the server.
/// Templated class because the errors can be both runtime and logic.
template <typename Base>
class ServerError : public Base {
 public:
  explicit ServerError(const Message& msg)
      : Base(msg.GetMessage()), msg_{msg} {}

  const Message& GetServerMessage() const { return msg_; }

  Message::Severity GetSeverity() const { return msg_.GetSeverity(); }
  SqlState GetSqlState() const { return msg_.GetSqlState(); }

 private:
  Message msg_;
};

using ServerLogicError = ServerError<LogicError>;
using ServerRuntimeError = ServerError<RuntimeError>;
//@}

//@{
/** @name Connection errors */
class ConnectionError : public RuntimeError {
  using RuntimeError::RuntimeError;
};

/// @brief Exception is thrown when a single connection fails to connect
class ConnectionFailed : public ConnectionError {
 public:
  explicit ConnectionFailed(const std::string& conninfo)
      : ConnectionError(conninfo + " Failed to connect to PostgreSQL server") {}
  ConnectionFailed(const std::string& conninfo, const std::string& message)
      : ConnectionError(conninfo + " " + message) {}
};

/// @brief Connection error reported by PostgreSQL server.
/// Doc: https://www.postgresql.org/docs/10/static/errcodes-appendix.html
/// Class 08 - Connection exception
class ServerConnectionError : public ServerError<ConnectionError> {
  using ServerError::ServerError;
};

/// @brief Indicates errors during pool operation
class PoolError : public ConnectionError {
  using ConnectionError::ConnectionError;
};

class ClusterUnavailable : public ConnectionError {
  using ConnectionError::ConnectionError;
};

/// @brief Error when invoking a libpq function
class CommandError : public ConnectionError {
  using ConnectionError::ConnectionError;
};
//@}

//@{
/** @name SQL errors */
//@{
/** @name Class 03 — SQL Statement Not Yet Complete */
/// A programming error, a statement is sent before other statement's results
/// are processed.
class SqlStatementNotYetComplete : public ServerLogicError {
  using ServerLogicError::ServerLogicError;
};
//@}

//@{
/** @name Class 09 — Triggered Action Exception */
class TriggeredActionException : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 0A — Feature Not Supported */
class FeatureNotSupported : public ServerLogicError {
  using ServerLogicError::ServerLogicError;
};
//@}

//@{
/** @name Class 0F - Locator Exception */
class LocatorException : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 0L - Invalid Grantor */
class InvalidGrantor : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 0P - Invalid Role Specification */
class InvalidRoleSpecification : public ServerLogicError {
  using ServerLogicError::ServerLogicError;
};
//@}

//@{
/** @name Class 0Z - Diagnostics Exception */
class DiagnosticsException : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 21 - Cardinality Violation */
class CardinalityViolation : public ServerLogicError {
  using ServerLogicError::ServerLogicError;
};
//@}

//@{
/** @name Class 22 — Data Exception */
/// @brief Base class for data exceptions
/// Doc: https://www.postgresql.org/docs/10/static/errcodes-appendix.html
class DataException : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 23 — Integrity Constraint Violation */
// TODO Shortcut accessors to respective message fields
/// @brief Base class for integrity constraint violation errors.
/// Doc: https://www.postgresql.org/docs/10/static/errcodes-appendix.html
class IntegrityConstraintViolation : public ServerRuntimeError {
 public:
  using ServerRuntimeError::ServerRuntimeError;

  std::string GetSchema() const { return GetServerMessage().GetSchema(); }
  std::string GetTable() const { return GetServerMessage().GetTable(); }
  std::string GetConstraint() const {
    return GetServerMessage().GetConstraint();
  }
};

class RestrictViolation : public IntegrityConstraintViolation {
  using IntegrityConstraintViolation::IntegrityConstraintViolation;
};

class NotNullViolation : public IntegrityConstraintViolation {
  using IntegrityConstraintViolation::IntegrityConstraintViolation;
};

class ForeignKeyViolation : public IntegrityConstraintViolation {
  using IntegrityConstraintViolation::IntegrityConstraintViolation;
};

class UniqueViolation : public IntegrityConstraintViolation {
  using IntegrityConstraintViolation::IntegrityConstraintViolation;
};

class CheckViolation : public IntegrityConstraintViolation {
  using IntegrityConstraintViolation::IntegrityConstraintViolation;
};

class ExclusionViolation : public IntegrityConstraintViolation {
  using IntegrityConstraintViolation::IntegrityConstraintViolation;
};

/// Class 27 - Triggered Data Change Violation
class TriggeredDataChangeViolation : public IntegrityConstraintViolation {
  using IntegrityConstraintViolation::IntegrityConstraintViolation;
};

/// Class 44 - WITH CHECK OPTION Violation
class WithCheckOptionViolation : public IntegrityConstraintViolation {
  using IntegrityConstraintViolation::IntegrityConstraintViolation;
};
//@}

//@{
/** @name Class 24 - Invalid Cursor State */
class InvalidCursorState : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 25 — Invalid Transaction State */
class InvalidTransactionState : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Invalid object name, several classes */
/// @brief Exception class for several Invalid * Name classes.
/// Class 26 - Invalid SQL Statement Name
/// Class 34 - Invalid Cursor Name
/// Class 3D - Invalid Catalogue Name
/// Class 3F - Invalid Schema Name
/// TODO Add documentation (links) on the error classes
/// TODO Split exception classes if needed based on documentation
class InvalidObjectName : public ServerLogicError {
  using ServerLogicError::ServerLogicError;
};
//@}

//@{
/** @name Class 28 - Invalid Authorisation Specification */
class InvalidAuthorizationSpecification : public ServerLogicError {
  using ServerLogicError::ServerLogicError;
};
//@}

//@{
/** @name Class 2B - Dependent Privilege Descriptors Still Exist */
class DependentPrivilegeDescriptorsStillExist : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 2D - Invalid Transaction Termination */
class InvalidTransactionTermination : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 38 - External Routine Exception */
class ExternalRoutineException : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 39 - External Routine Invocation Exception */
class ExternalRoutineInvocationException : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 3B - Savepoint Exception */
class SavepointException : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 2F — SQL Routine Exception */
class SqlRoutineException : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 40 — Transaction Rollback */
class TransactionRollback : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 42 — Syntax Error or Access Rule Violation */
class SyntaxError : public ServerLogicError {
  using ServerLogicError::ServerLogicError;
};

class AccessRuleViolation : public ServerLogicError {
  using ServerLogicError::ServerLogicError;
};

//@}

//@{
/** @name Class 53 - Insufficient Resources */
class InsufficientResources : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 54 - Program Limit Exceeded */
class ProgramLimitExceeded : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 55 - Object Not In Prerequisite State */
class ObjectNotInPrerequisiteState : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 57 - Operator Intervention */
class OperatorIntervention : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};

class QueryCanceled : public OperatorIntervention {
  using OperatorIntervention::OperatorIntervention;
};

class AdminShutdown : public OperatorIntervention {
  using OperatorIntervention::OperatorIntervention;
};

class CrashShutdown : public OperatorIntervention {
  using OperatorIntervention::OperatorIntervention;
};

class CannotConnectNow : public OperatorIntervention {
  using OperatorIntervention::OperatorIntervention;
};

class DatabaseDropped : public OperatorIntervention {
  using OperatorIntervention::OperatorIntervention;
};
//@}

//@{
/** @name Class 58 - System Error (errors external to PostgreSQL itself) */
class SystemError : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 72 — Snapshot Failure */
class SnapshotFailure : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class F0 — Configuration File Error */
class ConfigurationFileError : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class HV — Foreign Data Wrapper Error (SQL/MED) */
class FdwError : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class P0 — PL/pgSQL Error */
class PlPgSqlError : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class XX — Internal Error */
class InternalServerError : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}
//@}

//@{
/** @name Transaction errors */
class TransactionError : public LogicError {
  using LogicError::LogicError;
};

class AlreadyInTransaction : public TransactionError {
 public:
  AlreadyInTransaction()
      : TransactionError("Connection is already in a transaction block") {}
};

class NotInTransaction : public TransactionError {
 public:
  NotInTransaction()
      : TransactionError("Connection is not in a transaction block") {}
  NotInTransaction(const char* msg) : TransactionError(msg) {}
};
//@}

//@{
/** @name Result set usage errors */

class ResultSetError : public LogicError {
  using LogicError::LogicError;
};

/// @brief Result set has less rows than the requested row index.
class RowIndexOutOfBounds : public ResultSetError {
 public:
  RowIndexOutOfBounds(std::size_t index)
      : ResultSetError("Row index " + std::to_string(index) +
                       " is out of bounds") {}
};

/// @brief Result set has less columns that the requested index.
class FieldIndexOutOfBounds : public ResultSetError {
 public:
  FieldIndexOutOfBounds(std::size_t index)
      : ResultSetError("Field index " + std::to_string(index) +
                       " is out of bounds") {}
};

/// @brief Result set doesn't have field with the requested name.
class FieldNameDoesntExist : public ResultSetError {
 public:
  FieldNameDoesntExist(const std::string& name)
      : ResultSetError("Field name '" + name + "' doesn't exist") {}
};

/// @brief Data extraction from a null field value to a non-nullable type
/// requested.
class FieldValueIsNull : public ResultSetError {
 public:
  FieldValueIsNull(std::size_t field_index)
      : ResultSetError("Field " + std::to_string(field_index) +
                       "value is null") {}
};

/// @brief A value of a non-nullable type requested to be set null.
/// Can occur if io::traits::IsNullable for the type is specialised as
/// true_type, but io::traits::GetSetNull is not specialized appropriately.
class TypeCannotBeNull : public ResultSetError {
 public:
  TypeCannotBeNull(const std::string& type)
      : ResultSetError(type + " cannot be null") {}
};

/// @brief A field was requested to be parsed to a type that doesn't have an
/// appropriate parser.
/// This condition can be detected only at runtime.
class NoValueParser : public ResultSetError {
 public:
  NoValueParser(const std::string& type, io::DataFormat format)
      : ResultSetError(
            type + " doesn't have " +
            (format == io::DataFormat::kBinaryDataFormat ? "binary" : "text") +
            " parser") {}
};

/// @brief Buffer size is invalid for a fixed-size type.
/// Can occur when a wrong field type is requested for reply.
class InvalidInputBufferSize : public ResultSetError {
 public:
  InvalidInputBufferSize(std::size_t size, const std::string& message)
      : ResultSetError("Buffer size " + std::to_string(size) + " is invalid " +
                       message) {}
};

/// @brief Text buffer failed to parse to a requested type.
/// Can occur when a wrong field type is requested for reply or the data Postgre
/// text format doen't match text format in C++.
class TextParseFailure : public ResultSetError {
 public:
  TextParseFailure(const std::string& type, const std::string& buffer)
      : ResultSetError("Cannot parse `" + buffer + "` as " + type) {}
};

/// @brief A tuple was requested to be parsed out of a row that doesn't have
/// enough fields.
class InvalidTupleSizeRequested : public ResultSetError {
 public:
  InvalidTupleSizeRequested(std::size_t field_count, std::size_t tuple_size)
      : ResultSetError("Invalid tuple size requested. Field count " +
                       std::to_string(field_count) + " < tuple size " +
                       std::to_string(tuple_size)) {}
};

/// @brief A row was requested to be parsed based on field names/indexed,
/// the count of names/indexes doesn't match the tuple size.
class FieldTupleMismatch : public ResultSetError {
 public:
  FieldTupleMismatch(std::size_t field_count, std::size_t tuple_size)
      : ResultSetError("Invalid tuple size requested. Field count " +
                       std::to_string(field_count) + " != tuple size " +
                       std::to_string(tuple_size)) {}
};
//@}

//@{
/** @name Array errors */
/// @brief Base error when working with array types.
class ArrayError : public LogicError {
  using LogicError::LogicError;
};

/// @brief Array received from postgres has different dimensions from those of
/// C++ container.
class DimensionMismatch : public ArrayError {
 public:
  DimensionMismatch()
      : ArrayError("Array dimensions don't match dimensions of C++ type") {}
};

class InvalidDimensions : public ArrayError {
 public:
  InvalidDimensions(std::size_t expected, std::size_t actual)
      : ArrayError("Invalid dimension size " + std::to_string(actual) +
                   ". Expected " + std::to_string(expected)) {}
};

//@}

//@{
/** @name Enumeration type errors */
class EnumerationError : public LogicError {
  using LogicError::LogicError;
};

class InvalidEnumerationLiteral : EnumerationError {
 public:
  InvalidEnumerationLiteral(const std::string& type_name,
                            const std::string& literal)
      : EnumerationError("Invalid enumeration literal '" + literal +
                         "' for enum type '" + type_name + "'") {}
};

class InvalidEnumerationValue : EnumerationError {
 public:
  template <typename Enum>
  explicit InvalidEnumerationValue(Enum val)
      : EnumerationError(
            "Invalid enumeration value '" +
            std::to_string(static_cast<std::underlying_type_t<Enum>>(val)) +
            "' for enum type '" + ::utils::GetTypeName<Enum>()) {}
};
//@}

//@{
/** @name Misc exceptions */
class InvalidDSN : public RuntimeError {
 public:
  InvalidDSN(const std::string& dsn, const char* err)
      : RuntimeError("Invalid dsn '" + dsn + "': " + err) {}
};

class InvalidConfig : public RuntimeError {
  using RuntimeError::RuntimeError;
};

class NotImplemented : public LogicError {
  using LogicError::LogicError;
};

//@}

}  // namespace postgres
}  // namespace storages
