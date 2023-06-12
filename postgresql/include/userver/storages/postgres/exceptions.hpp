#pragma once

/// @file userver/storages/postgres/exceptions.hpp
/// @brief Postgres errors

#include <stdexcept>
#include <string_view>

#include <fmt/format.h>

#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/message.hpp>

#include <userver/compiler/demangle.hpp>
#include <userver/utils/underlying_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/**
 * @page pg_errors uPg: Postgres errors
 *
 * Base class for all PostgreSQL errors is Error which is derived from
 * std::runtime_error. This is done to simplify exception handling.
 *
 * There are two base types of errors: runtime (RuntimeError) and logic
 * (LogicError).
 *
 * **Logic errors** are a consequence of faulty logic within the program such as
 * violating logical preconditions or invariants and may be preventable by
 * correcting the program.
 *
 * **Runtime errors** are due to events beyond the scope of the program, such as
 * network failure, faulty configuration file, unique index violation etc. A
 * user can catch such an error and recover from it by reconnecting, providing
 * a decent default for configuration or modifying the key value.
 *
 * Both logic and runtime errors can contain a postgres server message
 * (Message). Those are ServerLogicError and ServerRuntimeError
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
 *     - DuplicatePreparedStatement
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
 *     - InvalidSqlStatementName
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
 *       - QueryCancelled
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
 *       - FieldNameDoesntExist
 *       - FieldTupleMismatch
 *       - FieldValueIsNull
 *       - InvalidBinaryBuffer
 *       - InvalidInputBufferSize
 *       - InvalidParserCategory
 *       - InvalidTupleSizeRequested
 *       - NonSingleColumnResultSet
 *       - NonSingleRowResultSet
 *       - NoBinaryParser
 *       - RowIndexOutOfBounds
 *       - TypeCannotBeNull
 *       - UnknownBufferCategory
 *     - UserTypeError
 *       - CompositeSizeMismatch
 *       - CompositeMemberTypeMismatch
 *     - ArrayError
 *       - DimensionMismatch
 *       - InvalidDimensions
 *     - NumericError
 *       - NumericOverflow
 *       - ValueIsNaN
 *       - InvalidRepresentation
 *     - InvalidInputFormat
 *     - EnumerationError
 *       - InvalidEnumerationLiteral
 *       - InvalidEnumerationValue
 *     - TransactionError
 *       - AlreadyInTransaction
 *       - NotInTransaction
 *     - UnsupportedInterval
 *     - BoundedRangeError
 *   - RuntimeError
 *     - ConnectionError
 *       - ClusterUnavailable
 *       - CommandError
 *       - ConnectionFailed
 *       - ServerConnectionError (contains a message from server)
 *       - ConnectionTimeoutError
 *     - ConnectionBusy
 *     - ConnectionInterrupted
 *     - PoolError
 *     - ClusterError
 *     - InvalidConfig
 *     - InvalidDSN
 *
 *
 * ----------
 *
 * @htmlonly <div class="bottom-nav"> @endhtmlonly
 * ⇦ @ref pg_user_row_types | @ref pg_topology ⇨
 * @htmlonly </div> @endhtmlonly
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
  explicit ConnectionFailed(const Dsn& dsn)
      : ConnectionError(fmt::format("{} Failed to connect to PostgreSQL server",
                                    DsnCutPassword(dsn))) {}
  ConnectionFailed(const Dsn& dsn, std::string_view message)
      : ConnectionError(fmt::format("{} {}", DsnCutPassword(dsn), message)) {}
};

/// @brief Connection error reported by PostgreSQL server.
/// Doc: https://www.postgresql.org/docs/12/static/errcodes-appendix.html
/// Class 08 - Connection exception
class ServerConnectionError : public ServerError<ConnectionError> {
  using ServerError::ServerError;
};

/// @brief Indicates errors during pool operation
class PoolError : public RuntimeError {
 public:
  PoolError(std::string_view msg, std::string_view db_name)
      : PoolError(fmt::format("{}, db_name={}", msg, db_name)) {}

  PoolError(std::string_view msg)
      : RuntimeError::RuntimeError(
            fmt::format("Postgres ConnectionPool error: {}", msg)) {}
};

class ClusterUnavailable : public ConnectionError {
  using ConnectionError::ConnectionError;
};

/// @brief Error when invoking a libpq function
class CommandError : public ConnectionError {
  using ConnectionError::ConnectionError;
};

/// @brief A network operation on a connection has timed out
class ConnectionTimeoutError : public ConnectionError {
  using ConnectionError::ConnectionError;
};

class ClusterError : public RuntimeError {
  using RuntimeError::RuntimeError;
};

/// @brief An attempt to make a query to server was made while there is another
/// query in flight.
class ConnectionBusy : public RuntimeError {
  using RuntimeError::RuntimeError;
};

/// @brief A network operation was interrupted by task cancellation.
class ConnectionInterrupted : public RuntimeError {
  using RuntimeError::RuntimeError;
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
/// Doc: https://www.postgresql.org/docs/12/static/errcodes-appendix.html
class DataException : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Class 23 — Integrity Constraint Violation */
// TODO Shortcut accessors to respective message fields
/// @brief Base class for integrity constraint violation errors.
/// Doc: https://www.postgresql.org/docs/12/static/errcodes-appendix.html
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
/** @name Class 26 - Invalid SQL Statement Name */
/// This exception is thrown in case a prepared statement doesn't exist
class InvalidSqlStatementName : public ServerRuntimeError {
  using ServerRuntimeError::ServerRuntimeError;
};
//@}

//@{
/** @name Invalid object name, several classes */
/// @brief Exception class for several Invalid * Name classes.
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

class DuplicatePreparedStatement : public ServerLogicError {
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

class QueryCancelled : public OperatorIntervention {
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
  NotInTransaction(const std::string& msg) : TransactionError(msg) {}
};
//@}

//@{
/** @name Result set usage errors */

class ResultSetError : public LogicError {
 public:
  ResultSetError(std::string msg)
      : LogicError::LogicError(msg), msg_(std::move(msg)) {}

  void AddMsgSuffix(std::string_view str) { msg_ += str; }

  const char* what() const noexcept override { return msg_.c_str(); }

 private:
  std::string msg_;
};

/// @brief Result set has less rows than the requested row index.
class RowIndexOutOfBounds : public ResultSetError {
 public:
  RowIndexOutOfBounds(std::size_t index)
      : ResultSetError(fmt::format("Row index {} is out of bounds", index)) {}
};

/// @brief Result set has less columns that the requested index.
class FieldIndexOutOfBounds : public ResultSetError {
 public:
  FieldIndexOutOfBounds(std::size_t index)
      : ResultSetError(fmt::format("Field index {} is out of bounds", index)) {}
};

/// @brief Result set doesn't have field with the requested name.
class FieldNameDoesntExist : public ResultSetError {
 public:
  FieldNameDoesntExist(std::string_view name)
      : ResultSetError(fmt::format("Field name '{}' doesn't exist", name)) {}
};

/// @brief Data extraction from a null field value to a non-nullable type
/// requested.
class FieldValueIsNull : public ResultSetError {
 public:
  template <typename T>
  FieldValueIsNull(std::size_t field_index, std::string_view field_name,
                   const T&)
      : ResultSetError(fmt::format("Field #{} name `{}` C++ type `{}` value is "
                                   "null, forgot `std::optional`?",
                                   field_index, field_name,
                                   compiler::GetTypeName<T>())) {}
};

/// @brief A value of a non-nullable type requested to be set null.
/// Can occur if io::traits::IsNullable for the type is specialised as
/// true_type, but io::traits::GetSetNull is not specialized appropriately.
class TypeCannotBeNull : public ResultSetError {
 public:
  TypeCannotBeNull(std::string_view type)
      : ResultSetError(fmt::format("{} cannot be null", type)) {}
};

/// @brief Field buffer contains different category of data than expected by
/// data parser.
class InvalidParserCategory : public ResultSetError {
 public:
  InvalidParserCategory(std::string_view type, io::BufferCategory parser,
                        io::BufferCategory buffer)
      : ResultSetError(fmt::format("Buffer category '{}' doesn't match the "
                                   "category of the parser '{}' for type '{}'",
                                   ToString(buffer), ToString(parser), type)) {}
};

/// @brief While checking result set types, failed to determine the buffer
/// category for a type oid.
/// The context string is formed by the ResultSet and will have the form
/// of 'result set field `foo` type `my_schema.bar` field `baz` array element'
class UnknownBufferCategory : public ResultSetError {
 public:
  UnknownBufferCategory(std::string_view context, Oid type_oid)
      : ResultSetError(
            fmt::format("Query {} doesn't have a parser. Type oid is {}",
                        context, type_oid)),
        type_oid{type_oid} {};

  const Oid type_oid;
};

/// @brief A field in a result set doesn't have a binary parser.
class NoBinaryParser : public ResultSetError {
  using ResultSetError::ResultSetError;
};

/// @brief Buffer size is invalid for a fixed-size type.
/// Can occur when a wrong field type is requested for reply.
class InvalidInputBufferSize : public ResultSetError {
 public:
  InvalidInputBufferSize(std::size_t size, std::string_view message)
      : ResultSetError(
            fmt::format("Buffer size {} is invalid: {}", size, message)) {}
};

/// @brief Binary buffer contains invalid data.
/// Can occur when parsing binary buffers containing multiple fields.
class InvalidBinaryBuffer : public ResultSetError {
 public:
  InvalidBinaryBuffer(const std::string& message)
      : ResultSetError("Invalid binary buffer: " + message) {}
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

/// @brief A row or result set requested to be treated as a single column, but
/// contains more than one column.
class NonSingleColumnResultSet : public ResultSetError {
 public:
  NonSingleColumnResultSet(std::size_t actual_size,
                           const std::string& type_name,
                           const std::string& func)
      : ResultSetError(
            "Parsing the row consisting of " + std::to_string(actual_size) +
            " columns as " + type_name +
            " is ambiguous as it can be used both for "
            "single column type and for a row. " +
            "Please use " + func + "<" + type_name + ">(kRowTag) or " + func +
            "<" + type_name + ">(kFieldTag) to resolve the ambiguity.") {}
};

/// @brief A result set containing a single row was expected
class NonSingleRowResultSet : public ResultSetError {
 public:
  explicit NonSingleRowResultSet(std::size_t actual_size)
      : ResultSetError(fmt::format("A single row result set was expected. "
                                   "Actual result set size is {}",
                                   actual_size)) {}
};

/// @brief A row was requested to be parsed based on field names/indexed,
/// the count of names/indexes doesn't match the tuple size.
class FieldTupleMismatch : public ResultSetError {
 public:
  FieldTupleMismatch(std::size_t field_count, std::size_t tuple_size)
      : ResultSetError(fmt::format(
            "Invalid tuple size requested. Field count {} != tuple size {}",
            field_count, tuple_size)) {}
};

//@}

//@{
/// @brief Base error when working with mapped types
class UserTypeError : public LogicError {
  using LogicError::LogicError;
};

/// @brief PostgreSQL composite type has different count of members from
/// the C++ counterpart.
class CompositeSizeMismatch : public UserTypeError {
 public:
  CompositeSizeMismatch(std::size_t pg_size, std::size_t cpp_size,
                        std::string_view cpp_type)
      : UserTypeError(
            fmt::format("Invalid composite type size. PostgreSQL type has {} "
                        "members, C++ type '{}' has {} members",
                        pg_size, cpp_type, cpp_size)) {}
};

/// @brief PostgreSQL composite type has different member type that the C++
/// mapping suggests.
class CompositeMemberTypeMismatch : public UserTypeError {
 public:
  CompositeMemberTypeMismatch(std::string_view pg_type_schema,
                              std::string_view pg_type_name,
                              std::string_view field_name, Oid pg_oid,
                              Oid user_oid)
      : UserTypeError(fmt::format(
            "Type mismatch for {}.{} field {}. In database the type "
            "oid is {}, user supplied type oid is {}",
            pg_type_schema, pg_type_name, field_name, pg_oid, user_oid)) {}
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
      : ArrayError(fmt::format("Invalid dimension size {}. Expected {}", actual,
                               expected)) {}
};

//@}

//@{
/** @name Numeric/decimal datatype errors */
class NumericError : public LogicError {
  using LogicError::LogicError;
};

/// Value in PostgreSQL binary buffer cannot be represented by a given C++ type
class NumericOverflow : public NumericError {
  using NumericError::NumericError;
};

/// PostgreSQL binary buffer contains NaN value, but the given C++ type doesn't
/// support NaN value
class ValueIsNaN : public NumericError {
  using NumericError::NumericError;
};

/// Integral representation for a numeric contains invalid data
class InvalidRepresentation : public NumericError {
  using NumericError::NumericError;
};
//@}

/// @brief Invalid format for input data.
///
/// Can occur when a numeric string representation cannot be parsed for sending
/// in binary buffers
class InvalidInputFormat : public LogicError {
  using LogicError::LogicError;
};

//@{
/** @name Enumeration type errors */
class EnumerationError : public LogicError {
  using LogicError::LogicError;
};

class InvalidEnumerationLiteral : public EnumerationError {
 public:
  InvalidEnumerationLiteral(std::string_view type_name,
                            std::string_view literal)
      : EnumerationError(
            fmt::format("Invalid enumeration literal '{}' for enum type '{}'",
                        literal, type_name)) {}
};

class InvalidEnumerationValue : public EnumerationError {
 public:
  template <typename Enum>
  explicit InvalidEnumerationValue(Enum val)
      : EnumerationError(
            fmt::format("Invalid enumeration value '{}' for enum type '{}'",
                        USERVER_NAMESPACE::utils::UnderlyingValue(val),
                        compiler::GetTypeName<Enum>())) {}
};
//@}

/// PostgreSQL interval datatype contains months field, which cannot be
/// converted to microseconds unambiguously
class UnsupportedInterval : public LogicError {
 public:
  UnsupportedInterval()
      : LogicError("PostgreSQL intervals containing months are not supported") {
  }
};

/// PostgreSQL range type has at least one end unbound
class BoundedRangeError : public LogicError {
 public:
  BoundedRangeError(std::string_view message)
      : LogicError(fmt::format(
            "PostgreSQL range violates bounded range invariant: {}", message)) {
  }
};

//@{
/** @name Misc exceptions */
class InvalidDSN : public RuntimeError {
 public:
  InvalidDSN(std::string_view dsn, std::string_view err)
      : RuntimeError(fmt::format("Invalid dsn '{}': {}", dsn, err)) {}
};

class InvalidConfig : public RuntimeError {
  using RuntimeError::RuntimeError;
};

class NotImplemented : public LogicError {
  using LogicError::LogicError;
};

//@}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
