#include <userver/storages/postgres/exceptions.hpp>

#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// clang-format off
//
// Done via:
// * Download
// https://raw.githubusercontent.com/postgres/postgres/master/src/include/catalog/pg_type.dat
// * replace `\n ` with ' '
// * sed -n "s/{ oid => '\([0-9]*\)'.*typname => '\([a-z_0-9]*\)'.*/.Case(\1, \"\2\")/p" pgtypes.txt
//
// clang-format on
constexpr utils::TrivialBiMap kOidToReadableName = [](auto selector) {
  return selector()
      .Case(16, "bool")
      .Case(17, "bytea")
      .Case(18, "char")
      .Case(19, "name")
      .Case(20, "int8")
      .Case(21, "int2")
      .Case(22, "int2vector")
      .Case(23, "int4")
      .Case(24, "regproc")
      .Case(25, "text")
      .Case(26, "oid")
      .Case(27, "tid")
      .Case(28, "xid")
      .Case(29, "cid")
      .Case(30, "oidvector")
      .Case(71, "pg_type")
      .Case(75, "pg_attribute")
      .Case(81, "pg_proc")
      .Case(83, "pg_class")
      .Case(114, "json")
      .Case(142, "xml")
      .Case(194, "pg_node_tree")
      .Case(3361, "pg_ndistinct")
      .Case(3402, "pg_dependencies")
      .Case(5017, "pg_mcv_list")
      .Case(32, "pg_ddl_command")
      .Case(5069, "xid8")
      .Case(600, "point")
      .Case(601, "lseg")
      .Case(602, "path")
      .Case(603, "box")
      .Case(604, "polygon")
      .Case(628, "line")
      .Case(700, "float4")
      .Case(701, "float8")
      .Case(705, "unknown")
      .Case(718, "circle")
      .Case(790, "money")
      .Case(829, "macaddr")
      .Case(869, "inet")
      .Case(650, "cidr")
      .Case(774, "macaddr8")
      .Case(1033, "aclitem")
      .Case(1042, "bpchar")
      .Case(1043, "varchar")
      .Case(1082, "date")
      .Case(1083, "time")
      .Case(1114, "timestamp")
      .Case(1184, "timestamptz")
      .Case(1186, "interval")
      .Case(1266, "timetz")
      .Case(1560, "bit")
      .Case(1562, "varbit")
      .Case(1700, "numeric")
      .Case(1790, "refcursor")
      .Case(2202, "regprocedure")
      .Case(2203, "regoper")
      .Case(2204, "regoperator")
      .Case(2205, "regclass")
      .Case(4191, "regcollation")
      .Case(2206, "regtype")
      .Case(4096, "regrole")
      .Case(4089, "regnamespace")
      .Case(2950, "uuid")
      .Case(3220, "pg_lsn")
      .Case(3614, "tsvector")
      .Case(3642, "gtsvector")
      .Case(3615, "tsquery")
      .Case(3734, "regconfig")
      .Case(3769, "regdictionary")
      .Case(3802, "jsonb")
      .Case(4072, "jsonpath")
      .Case(2970, "txid_snapshot")
      .Case(5038, "pg_snapshot")
      .Case(3904, "int4range")
      .Case(3906, "numrange")
      .Case(3908, "tsrange")
      .Case(3910, "tstzrange")
      .Case(3912, "daterange")
      .Case(3926, "int8range")
      .Case(4451, "int4multirange")
      .Case(4532, "nummultirange")
      .Case(4533, "tsmultirange")
      .Case(4534, "tstzmultirange")
      .Case(4535, "datemultirange")
      .Case(4536, "int8multirange")
      .Case(2249, "record")
      .Case(2287, "_record")
      .Case(2275, "cstring")
      .Case(2276, "any")
      .Case(2277, "anyarray")
      .Case(2278, "void")
      .Case(2279, "trigger")
      .Case(3838, "event_trigger")
      .Case(2280, "language_handler")
      .Case(2281, "internal")
      .Case(2283, "anyelement")
      .Case(2776, "anynonarray")
      .Case(3500, "anyenum")
      .Case(3115, "fdw_handler")
      .Case(325, "index_am_handler")
      .Case(3310, "tsm_handler")
      .Case(269, "table_am_handler")
      .Case(3831, "anyrange")
      .Case(5077, "anycompatible")
      .Case(5078, "anycompatiblearray")
      .Case(5079, "anycompatiblenonarray")
      .Case(5080, "anycompatiblerange")
      .Case(4537, "anymultirange")
      .Case(4538, "anycompatiblemultirange")
      .Case(4600, "pg_brin_bloom_summary")
      .Case(4601, "pg_brin_minmax_multi_summary");
};

}  // anonymous namespace

namespace storages::postgres {

namespace {

std::string OidPrettyPrint(Oid oid) {
  const auto name = kOidToReadableName.TryFind(oid);
  if (name) {
    return fmt::format("is '{}' (oid: {})", *name, oid);
  }
  return fmt::format("oid is {}", oid);
}

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
          "Type mismatch for '{}.{}' field '{}'. In database the type "
          "{}, user supplied type {}",
          pg_type_schema, pg_type_name, field_name, OidPrettyPrint(pg_oid),
          OidPrettyPrint(user_oid))) {}

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
