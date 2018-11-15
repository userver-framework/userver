#include <storages/postgres/detail/result_wrapper.hpp>

#include <logging/log.hpp>

#ifndef PG_DIAG_SEVERITY_NONLOCALIZED
#define PG_DIAG_SEVERITY_NONLOCALIZED 'V'
#endif

namespace storages {
namespace postgres {
namespace detail {

namespace {

constexpr std::pair<const char*, int> kExtraErrorFields[]{
    {"pg_severity", PG_DIAG_SEVERITY_NONLOCALIZED},
    {"pg_sqlstate", PG_DIAG_SQLSTATE},
    {"pg_detail", PG_DIAG_MESSAGE_DETAIL},
    {"pg_hint", PG_DIAG_MESSAGE_HINT},
    {"pg_statement_position", PG_DIAG_STATEMENT_POSITION},
    {"pg_internal_query", PG_DIAG_INTERNAL_QUERY},
    {"pg_internal_position", PG_DIAG_INTERNAL_POSITION},
    {"pg_context", PG_DIAG_CONTEXT},
    {"pg_schema", PG_DIAG_SCHEMA_NAME},
    {"pg_table", PG_DIAG_TABLE_NAME},
    {"pg_column", PG_DIAG_COLUMN_NAME},
    {"pg_datatype", PG_DIAG_DATATYPE_NAME},
    {"pg_constraint", PG_DIAG_CONSTRAINT_NAME}};

}  // namespace

ExecStatusType ResultWrapper::GetStatus() const {
  return PQresultStatus(handle_.get());
}

std::size_t ResultWrapper::RowCount() const { return PQntuples(handle_.get()); }

std::size_t ResultWrapper::FieldCount() const {
  return PQnfields(handle_.get());
}

std::size_t ResultWrapper::IndexOfName(const std::string& name) const {
  auto n = PQfnumber(handle_.get(), name.c_str());
  if (n < 0) return ResultSet::npos;
  return n;
}

FieldDescription ResultWrapper::GetFieldDescription(std::size_t col) const {
  return {col,
          GetFieldTypeOid(col),
          std::string{PQfname(handle_.get(), col)},
          GetFieldFormat(col),
          PQftable(handle_.get(), col),
          PQftablecol(handle_.get(), col),
          PQfsize(handle_.get(), col),
          PQfmod(handle_.get(), col)};
}

bool ResultWrapper::IsFieldNull(std::size_t row, std::size_t col) const {
  return PQgetisnull(handle_.get(), row, col);
}

io::DataFormat ResultWrapper::GetFieldFormat(std::size_t col) const {
  return static_cast<io::DataFormat>(PQfformat(handle_.get(), col));
}

Oid ResultWrapper::GetFieldTypeOid(std::size_t col) const {
  return PQftype(handle_.get(), col);
}

std::size_t ResultWrapper::GetFieldLength(std::size_t row,
                                          std::size_t col) const {
  return PQgetlength(handle_.get(), row, col);
}

io::FieldBuffer ResultWrapper::GetFieldBuffer(std::size_t row,
                                              std::size_t col) const {
  return io::FieldBuffer{IsFieldNull(row, col), GetFieldFormat(col),
                         GetFieldLength(row, col),
                         PQgetvalue(handle_.get(), row, col)};
}

std::string ResultWrapper::GetErrorMessage() const {
  return {PQresultErrorMessage(handle_.get())};
}

std::string ResultWrapper::GetDetailErrorMessage() const {
  return GetMessageField(PG_DIAG_MESSAGE_DETAIL);
  ;
}

std::string ResultWrapper::GetMessageSeverityString() const {
  return GetMessageField(PG_DIAG_SEVERITY);
}

Message::Severity ResultWrapper::GetMessageSeverity() const {
  return Message::SeverityFromString(
      GetMessageField(PG_DIAG_SEVERITY_NONLOCALIZED));
}

std::string ResultWrapper::GetSqlCode() const {
  return GetMessageField(PG_DIAG_SQLSTATE);
}

SqlState ResultWrapper::GetSqlState() const {
  return SqlStateFromString(
      PQresultErrorField(handle_.get(), PG_DIAG_SQLSTATE));
}

std::string ResultWrapper::GetMessageSchema() const {
  return GetMessageField(PG_DIAG_SCHEMA_NAME);
}
std::string ResultWrapper::GetMessageTable() const {
  return GetMessageField(PG_DIAG_TABLE_NAME);
}
std::string ResultWrapper::GetMessageColumn() const {
  return GetMessageField(PG_DIAG_COLUMN_NAME);
}
std::string ResultWrapper::GetMessageDatatype() const {
  return GetMessageField(PG_DIAG_DATATYPE_NAME);
}
std::string ResultWrapper::GetMessageConstraint() const {
  return GetMessageField(PG_DIAG_CONSTRAINT_NAME);
}

std::string ResultWrapper::GetMessageField(int fieldcode) const {
  return {PQresultErrorField(handle_.get(), fieldcode)};
}

logging::LogExtra ResultWrapper::GetMessageLogExtra() const {
  logging::LogExtra log_extra;

  for (const auto& f : kExtraErrorFields) {
    auto msg = PQresultErrorField(handle_.get(), f.second);
    if (msg) {
      log_extra.Extend({f.first, std::string{msg}});
    }
  }
  return log_extra;
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
