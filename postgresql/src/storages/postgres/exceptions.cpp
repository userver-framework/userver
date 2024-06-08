#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace {

std::string GetInvalidParserCategoryMessage(std::string_view type,
                                            io::BufferCategory parser,
                                            io::BufferCategory buffer) {
  std::string message = fmt::format(
      "Buffer category '{}' doesn't match the "
      "category of the parser '{}' for type '{}'.",
      ToString(buffer), ToString(parser), type);
  if (parser == io::BufferCategory::kCompositeBuffer &&
      buffer == io::BufferCategory::kPlainBuffer) {
    message += fmt::format(
        " Consider using different variable type (not '{}') to store result, "
        "passing storages::postgres::kRowTag to function args for this field "
        "or explicitly cast to expected type in SQL query.",
        type);
  }
  if (parser == io::BufferCategory::kPlainBuffer &&
      buffer == io::BufferCategory::kCompositeBuffer) {
    message += fmt::format(
        " Consider using different variable type (not '{}') "
        "to store complex result or split result "
        "tuple with 'UNNEST' in SQL query.",
        type);
  }
  return message;
}

}  // namespace

ConnectionFailed::ConnectionFailed(const Dsn& dsn)
    : ConnectionError(fmt::format("{} Failed to connect to PostgreSQL server",
                                  DsnCutPassword(dsn))) {}

ConnectionFailed::ConnectionFailed(const Dsn& dsn, std::string_view message)
    : ConnectionError(fmt::format("{} {}", DsnCutPassword(dsn), message)) {}

PoolError::PoolError(std::string_view msg, std::string_view db_name)
    : PoolError(fmt::format("{}, db_name={}", msg, db_name)) {}

PoolError::PoolError(std::string_view msg)
    : RuntimeError::RuntimeError(
          fmt::format("Postgres ConnectionPool error: {}", msg)) {}

std::string IntegrityConstraintViolation::GetSchema() const {
  return GetServerMessage().GetSchema();
}

std::string IntegrityConstraintViolation::GetTable() const {
  return GetServerMessage().GetTable();
}

std::string IntegrityConstraintViolation::GetConstraint() const {
  return GetServerMessage().GetConstraint();
}

AlreadyInTransaction::AlreadyInTransaction()
    : TransactionError("Connection is already in a transaction block") {}

NotInTransaction::NotInTransaction()
    : TransactionError("Connection is not in a transaction block") {}

NotInTransaction::NotInTransaction(const std::string& msg)
    : TransactionError(msg) {}

TransactionForceRollback::TransactionForceRollback()
    : TransactionError("Force rollback due to Testpoint response") {}

TransactionForceRollback::TransactionForceRollback(const std::string& msg)
    : TransactionError(msg) {}

ResultSetError::ResultSetError(std::string msg)
    : LogicError::LogicError(msg), msg_(std::move(msg)) {}

void ResultSetError::AddMsgSuffix(std::string_view str) { msg_ += str; }

const char* ResultSetError::what() const noexcept { return msg_.c_str(); }

RowIndexOutOfBounds::RowIndexOutOfBounds(std::size_t index)
    : ResultSetError(fmt::format("Row index {} is out of bounds", index)) {}

FieldIndexOutOfBounds::FieldIndexOutOfBounds(std::size_t index)
    : ResultSetError(fmt::format("Field index {} is out of bounds", index)) {}

FieldNameDoesntExist::FieldNameDoesntExist(std::string_view name)
    : ResultSetError(fmt::format("Field name '{}' doesn't exist", name)) {}

TypeCannotBeNull::TypeCannotBeNull(std::string_view type)
    : ResultSetError(fmt::format("{} cannot be null", type)) {}

InvalidParserCategory::InvalidParserCategory(std::string_view type,
                                             io::BufferCategory parser,
                                             io::BufferCategory buffer)
    : ResultSetError(GetInvalidParserCategoryMessage(type, parser, buffer)) {}

UnknownBufferCategory::UnknownBufferCategory(std::string_view context,
                                             Oid type_oid)
    : ResultSetError(fmt::format(
          "Query {} doesn't have a parser. Type oid is {}", context, type_oid)),
      type_oid{type_oid} {}

InvalidInputBufferSize::InvalidInputBufferSize(std::size_t size,
                                               std::string_view message)
    : ResultSetError(
          fmt::format("Buffer size {} is invalid: {}", size, message)) {}

InvalidBinaryBuffer::InvalidBinaryBuffer(const std::string& message)
    : ResultSetError("Invalid binary buffer: " + message) {}

InvalidTupleSizeRequested::InvalidTupleSizeRequested(std::size_t field_count,
                                                     std::size_t tuple_size)
    : ResultSetError("Invalid tuple size requested. Field count " +
                     std::to_string(field_count) + " < tuple size " +
                     std::to_string(tuple_size)) {}

NonSingleColumnResultSet::NonSingleColumnResultSet(std::size_t actual_size,
                                                   const std::string& type_name,
                                                   const std::string& func)
    : ResultSetError("Parsing the row consisting of " +
                     std::to_string(actual_size) + " columns as " + type_name +
                     " is ambiguous as it can be used both for "
                     "single column type and for a row. " +
                     "Please use " + func + "<" + type_name + ">(kRowTag) or " +
                     func + "<" + type_name +
                     ">(kFieldTag) to resolve the ambiguity.") {}

NonSingleRowResultSet::NonSingleRowResultSet(std::size_t actual_size)
    : ResultSetError(fmt::format("A single row result set was expected. "
                                 "Actual result set size is {}",
                                 actual_size)) {}

FieldTupleMismatch::FieldTupleMismatch(std::size_t field_count,
                                       std::size_t tuple_size)
    : ResultSetError(fmt::format(
          "Invalid tuple size requested. Field count {} != tuple size {}",
          field_count, tuple_size)) {}

CompositeSizeMismatch::CompositeSizeMismatch(std::size_t pg_size,
                                             std::size_t cpp_size,
                                             std::string_view cpp_type)
    : UserTypeError(
          fmt::format("Invalid composite type size. PostgreSQL type has {} "
                      "members, C++ type '{}' has {} members",
                      pg_size, cpp_type, cpp_size)) {}

CompositeMemberTypeMismatch::CompositeMemberTypeMismatch(
    std::string_view pg_type_schema, std::string_view pg_type_name,
    std::string_view field_name, Oid pg_oid, Oid user_oid)
    : UserTypeError(fmt::format(
          "Type mismatch for {}.{} field {}. In database the type "
          "oid is {}, user supplied type oid is {}",
          pg_type_schema, pg_type_name, field_name, pg_oid, user_oid)) {}

DimensionMismatch::DimensionMismatch()
    : ArrayError("Array dimensions don't match dimensions of C++ type") {}

InvalidDimensions::InvalidDimensions(std::size_t expected, std::size_t actual)
    : ArrayError(fmt::format("Invalid dimension size {}. Expected {}", actual,
                             expected)) {}

InvalidEnumerationLiteral::InvalidEnumerationLiteral(std::string_view type_name,
                                                     std::string_view literal)
    : EnumerationError(
          fmt::format("Invalid enumeration literal '{}' for enum type '{}'",
                      literal, type_name)) {}

UnsupportedInterval::UnsupportedInterval()
    : LogicError("PostgreSQL intervals containing months are not supported") {}

BoundedRangeError::BoundedRangeError(std::string_view message)
    : LogicError(fmt::format(
          "PostgreSQL range violates bounded range invariant: {}", message)) {}

BitStringOverflow::BitStringOverflow(std::size_t actual, std::size_t expected)
    : BitStringError(fmt::format("Invalid bit container size {}. Expected {}",
                                 actual, expected)) {}

InvalidBitStringRepresentation::InvalidBitStringRepresentation()
    : BitStringError("Invalid bit or bit varying type representation") {}

InvalidDSN::InvalidDSN(std::string_view dsn, std::string_view err)
    : RuntimeError(fmt::format("Invalid dsn '{}': {}", dsn, err)) {}

IpAddressInvalidFormat::IpAddressInvalidFormat(std::string_view str)
    : IpAddressError(fmt::format("IP address invalid format. {}", str)) {}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
