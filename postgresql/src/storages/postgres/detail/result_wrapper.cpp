#include <storages/postgres/detail/result_wrapper.hpp>

#include <fmt/format.h>
#include <boost/algorithm/string/join.hpp>

#include <storages/postgres/io/traits.hpp>

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

struct CurrentContext {
  CurrentContext(std::vector<std::string>& context, std::string&& ctx)
      : context_{context} {
    context_.emplace_back(std::move(ctx));
  }
  ~CurrentContext() { context_.pop_back(); }

 private:
  std::vector<std::string>& context_;
};

void AddTypeBufferCategories(Oid data_type, const UserTypes& types,
                             io::TypeBufferCategory& cats,
                             std::vector<std::string>& context) {
  if (cats.count(data_type)) {
    return;
  }
  auto cat = types.GetBufferCategory(data_type);
  if (cat == io::BufferCategory::kNoParser) {
    throw UnknownBufferCategory(boost::join(context, " "), data_type);
  }
  cats.insert(std::make_pair(data_type, cat));
  if (cat == io::BufferCategory::kArrayBuffer) {
    // Recursively add buffer category for array element
    auto elem_oid = types.FindElementOid(data_type);
    CurrentContext ctx{context, "array element"};
    AddTypeBufferCategories(elem_oid, types, cats, context);
  } else if (cat == io::BufferCategory::kCompositeBuffer) {
    // Recursively add buffer categories for data members
    const auto& type_desc = types.GetCompositeDescription(data_type);
    auto type_name = types.FindName(data_type);
    CurrentContext ctx{context, fmt::format("type `{}`", type_name.ToString())};
    auto n_fields = type_desc.Size();
    for (std::size_t f_no = 0; f_no < n_fields; ++f_no) {
      CurrentContext ctx{context,
                         fmt::format("field `{}`", type_desc[f_no].name)};
      AddTypeBufferCategories(type_desc[f_no].type, types, cats, context);
    }
  }
}

}  // namespace

void ResultWrapper::FillBufferCategories(const UserTypes& types) {
  buffer_categories_.clear();
  auto n_fields = FieldCount();
  for (std::size_t f_no = 0; f_no < n_fields; ++f_no) {
    auto data_type = GetFieldTypeOid(f_no);

    std::vector<std::string> context{
        fmt::format("result set field `{}`", GetFieldName(f_no))};

    AddTypeBufferCategories(data_type, types, buffer_categories_, context);
  }
}

ExecStatusType ResultWrapper::GetStatus() const {
  return PQresultStatus(handle_.get());
}

std::size_t ResultWrapper::RowCount() const { return PQntuples(handle_.get()); }

std::size_t ResultWrapper::FieldCount() const {
  return PQnfields(handle_.get());
}

std::string ResultWrapper::CommandStatus() const {
  return PQcmdStatus(handle_.get());
}

std::size_t ResultWrapper::RowsAffected() const {
  auto str = PQcmdTuples(handle_.get());
  if (str) {
    char* endptr = nullptr;
    auto val = std::strtoll(str, &endptr, 10);
    if (endptr == str) {
      return 0;
    }
    if (val < 0) {
      // This is very strange and actually shouldn't happen, but just in case
      return 0;
    }
    return val;
  }
  return 0;
}

std::size_t ResultWrapper::IndexOfName(const std::string& name) const {
  auto n = PQfnumber(handle_.get(), name.c_str());
  if (n < 0) return ResultSet::npos;
  return n;
}

std::string_view ResultWrapper::GetFieldName(std::size_t col) const {
  auto name = PQfname(handle_.get(), col);
  if (name) {
    return {name};
  }
  throw ResultSetError{"Column with index " + std::to_string(col) +
                       " doesn't have a name in result set description"};
}

FieldDescription ResultWrapper::GetFieldDescription(std::size_t col) const {
  return {col,
          GetFieldTypeOid(col),
          std::string{GetFieldName(col)},
          PQftable(handle_.get(), col),
          PQftablecol(handle_.get(), col),
          PQfsize(handle_.get(), col),
          PQfmod(handle_.get(), col)};
}

bool ResultWrapper::IsFieldNull(std::size_t row, std::size_t col) const {
  return PQgetisnull(handle_.get(), row, col);
}

Oid ResultWrapper::GetFieldTypeOid(std::size_t col) const {
  return PQftype(handle_.get(), col);
}

io::BufferCategory ResultWrapper::GetFieldBufferCategory(
    std::size_t col) const {
  auto data_type = GetFieldTypeOid(col);
  if (auto f = buffer_categories_.find(data_type);
      f != buffer_categories_.end()) {
    return f->second;
  }
  return io::BufferCategory::kNoParser;
}

std::size_t ResultWrapper::GetFieldLength(std::size_t row,
                                          std::size_t col) const {
  return PQgetlength(handle_.get(), row, col);
}

io::FieldBuffer ResultWrapper::GetFieldBuffer(std::size_t row,
                                              std::size_t col) const {
  if (PQfformat(handle_.get(), col) != io::kPgBinaryDataFormat) {
    throw ResultSetError{"Column with index " + std::to_string(col) +
                         " has text format" +
                         to_string(boost::stacktrace::stacktrace{})};
  }
  return io::FieldBuffer{IsFieldNull(row, col), GetFieldBufferCategory(col),
                         GetFieldLength(row, col),
                         reinterpret_cast<const std::uint8_t*>(
                             PQgetvalue(handle_.get(), row, col))};
}

std::string ResultWrapper::GetErrorMessage() const {
  auto msg = PQresultErrorMessage(handle_.get());
  return {msg ? msg : "no error message"};
}

std::string ResultWrapper::GetPrimaryErrorMessage() const {
  return GetMessageField(PG_DIAG_MESSAGE_PRIMARY);
}

std::string ResultWrapper::GetDetailErrorMessage() const {
  return GetMessageField(PG_DIAG_MESSAGE_DETAIL);
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
  auto msg = PQresultErrorField(handle_.get(), PG_DIAG_SQLSTATE);
  return msg ? SqlStateFromString(msg) : SqlState::kUnknownState;
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
