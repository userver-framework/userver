#include <storages/postgres/detail/result_wrapper.hpp>

#include <fmt/compile.h>
#include <fmt/format.h>
#include <boost/container/small_vector.hpp>
#include <boost/stacktrace/stacktrace.hpp>

#include <userver/logging/log.hpp>
#include <userver/logging/stacktrace_cache.hpp>

#include <storages/postgres/detail/pg_message_severity.hpp>
#include <userver/storages/postgres/io/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

namespace {

const std::string kSeverityLogExtraKey = "pg_severity";

constexpr std::pair<const char*, int> kExtraErrorFields[]{
    // pg_severity is processed separately
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

// A scope we use to manage a type-name "path".
// Only used when we've encountered an unknown buffer category.
class TypeNameScope final {
 public:
  template <typename Fn>
  TypeNameScope(std::vector<std::string>& context, Fn&& ctx_factory)
      : context_{context} {
    context_.emplace_back(ctx_factory());
  }
  ~TypeNameScope() { context_.pop_back(); }

  static bool Fail(const std::vector<std::string>& context, Oid data_type) {
    throw UnknownBufferCategory(fmt::to_string(fmt::join(context, " ")),
                                data_type);
  }

 private:
  std::vector<std::string>& context_;
};

// A no-op scope we use when validating buffer categories.
// If we encounter a parsing failure the TypeNameScope is used.
class NoopTypeNameScope final {
 public:
  template <typename Fn>
  NoopTypeNameScope(std::vector<std::string>&, Fn&&) {}

  static bool Fail(const std::vector<std::string>&, Oid) { return false; }
};

template <typename ScopeType>
bool DoAddTypeBufferCategories(Oid data_type, const UserTypes& types,
                               io::TypeBufferCategory& cats,
                               std::vector<std::string>& context) {
  if (cats.count(data_type)) {
    return true;
  }
  auto cat = io::GetBufferCategory(static_cast<io::PredefinedOids>(data_type));
  if (cat == io::BufferCategory::kNoParser) {
    cat = types.GetBufferCategory(data_type);
    if (cat == io::BufferCategory::kNoParser) {
      return ScopeType::Fail(context, data_type);
    }
  }

  cats.insert(std::make_pair(data_type, cat));

  if (cat == io::BufferCategory::kArrayBuffer) {
    // Recursively add buffer category for array element
    auto elem_oid = types.FindElementOid(data_type);
    const ScopeType path_scope{context,
                               [] { return std::string{"array element"}; }};
    return DoAddTypeBufferCategories<ScopeType>(elem_oid, types, cats, context);
  } else if (cat == io::BufferCategory::kCompositeBuffer) {
    if (data_type == static_cast<Oid>(io::PredefinedOids::kRecord)) {
      // record is opaque and doesn't have a description
      // load buffer categories for all user types
      const auto& user_categories = types.GetTypeBufferCategories();
      cats.insert(user_categories.begin(), user_categories.end());
      return true;
    }

    // Recursively add buffer categories for data members
    const auto& type_desc = types.GetCompositeDescription(data_type);
    auto type_name = types.FindName(data_type);
    const ScopeType path_scope{context, [&type_name] {
                                 return fmt::format(FMT_COMPILE("type `{}`"),
                                                    type_name.ToString());
                               }};
    auto n_fields = type_desc.Size();
    for (std::size_t f_no = 0; f_no < n_fields; ++f_no) {
      const ScopeType child_scope{
          context, [&type_desc, f_no] {
            return fmt::format(FMT_COMPILE("field `{}`"), type_desc[f_no].name);
          }};
      if (!DoAddTypeBufferCategories<ScopeType>(type_desc[f_no].type, types,
                                                cats, context)) {
        return false;
      }
    }
  }

  return true;
}

template <typename ScopeType, typename GetFieldTypeOidFn,
          typename GetFieldNameFn>
bool AddTypeBufferCategories(std::size_t n_fields,
                             const GetFieldTypeOidFn& get_field_type_oid_fn,
                             const GetFieldNameFn& get_field_name_fn,
                             const UserTypes& types,
                             io::TypeBufferCategory& buffer_categories) {
  std::vector<std::string> context{};

  for (std::size_t f_no = 0; f_no < n_fields; ++f_no) {
    auto data_type = get_field_type_oid_fn(f_no);
    ScopeType path_start_scope{context, [&get_field_name_fn, f_no] {
                                 return fmt::format("result set field `{}`",
                                                    get_field_name_fn(f_no));
                               }};

    if (!DoAddTypeBufferCategories<ScopeType>(data_type, types,
                                              buffer_categories, context)) {
      return false;
    }
  }

  return true;
}

}  // namespace

struct ResultWrapper::CachedFieldBufferCategories final {
  boost::container::small_vector<io::BufferCategory, 16> data;
};

ResultWrapper::ResultWrapper(ResultHandle&& res) : handle_{std::move(res)} {
  UASSERT(handle_);
}

ResultWrapper::~ResultWrapper() = default;

void ResultWrapper::FillBufferCategories(const UserTypes& types) {
  buffer_categories_.clear();

  const auto n_fields = FieldCount();
  buffer_categories_.reserve(n_fields);

  const auto oid_fn = [this](auto f_no) { return GetFieldTypeOid(f_no); };
  const auto name_fn = [this](auto f_no) { return GetFieldName(f_no); };

  // We have to throw an informative exception if some (potentially deeply
  // nested) type doesn't have a parser.
  //
  // The problem: for that exception to be of use we have to preserve a path to
  // the field, which implies a vector of path chunks, lots of fmt::format-s,
  // allocations etc., and is NEVER used when everything goes according to plan.
  //
  // The solution: run the recursive parsing twice, the first attempt is
  // performed without preserving paths, and iff it fails the second attempt
  // re-runs the process from scratch with all the info it needs,
  // and is guaranteed to throw.
  if (!AddTypeBufferCategories<NoopTypeNameScope>(n_fields, oid_fn, name_fn,
                                                  types, buffer_categories_)) {
    buffer_categories_.clear();
    AddTypeBufferCategories<TypeNameScope>(n_fields, oid_fn, name_fn, types,
                                           buffer_categories_);
    UINVARIANT(false, "Should be unreachable");
  }

  cached_buffer_categories_->data.resize(n_fields);
  for (std::size_t f_no = 0; f_no < n_fields; ++f_no) {
    const auto data_type = GetFieldTypeOid(f_no);
    const auto f = buffer_categories_.find(data_type);

    cached_buffer_categories_->data[f_no] =
        (f == buffer_categories_.end() ? io::BufferCategory::kNoParser
                                       : f->second);
  }
}

ExecStatusType ResultWrapper::GetStatus() const {
  return PQresultStatus(handle_.get());
}

std::size_t ResultWrapper::RowCount() const { return PQntuples(handle_.get()); }

std::size_t ResultWrapper::FieldCount() const {
  return PQnfields(handle_.get());
}

const io::TypeBufferCategory& ResultWrapper::GetTypeBufferCategories() const {
  return buffer_categories_;
}

void ResultWrapper::SetTypeBufferCategories(const ResultWrapper& description) {
  buffer_categories_ = description.buffer_categories_;
  cached_buffer_categories_ = description.cached_buffer_categories_;
}

std::string ResultWrapper::CommandStatus() const {
  return PQcmdStatus(handle_.get());
}

std::size_t ResultWrapper::RowsAffected() const {
  auto* str = PQcmdTuples(handle_.get());
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
  auto* name = PQfname(handle_.get(), col);
  if (name) {
    return {name};
  }
  throw ResultSetError{fmt::format(
      "Column with index {} doesn't have a name in result set description",
      col)};
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
  return cached_buffer_categories_->data[col];
}

std::size_t ResultWrapper::GetFieldLength(std::size_t row,
                                          std::size_t col) const {
  return PQgetlength(handle_.get(), row, col);
}

io::FieldBuffer ResultWrapper::GetFieldBuffer(std::size_t row,
                                              std::size_t col) const {
  if (PQfformat(handle_.get(), col) != io::kPgBinaryDataFormat) {
    throw ResultSetError{
        fmt::format("Column with index {} has text format\n", col) +
        logging::stacktrace_cache::to_string(boost::stacktrace::stacktrace{})};
  }
  return io::FieldBuffer{IsFieldNull(row, col), GetFieldBufferCategory(col),
                         GetFieldLength(row, col),
                         reinterpret_cast<const std::uint8_t*>(
                             PQgetvalue(handle_.get(), row, col))};
}

std::string ResultWrapper::GetErrorMessage() const {
  auto* msg = PQresultErrorMessage(handle_.get());
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
  return Message::SeverityFromString(GetMachineReadableSeverity(handle_.get()));
}

std::string ResultWrapper::GetSqlCode() const {
  return GetMessageField(PG_DIAG_SQLSTATE);
}

SqlState ResultWrapper::GetSqlState() const {
  auto* msg = PQresultErrorField(handle_.get(), PG_DIAG_SQLSTATE);
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

  auto severity = GetMachineReadableSeverity(handle_.get());
  if (!severity.empty()) {
    log_extra.Extend(kSeverityLogExtraKey, std::string{severity});
  }

  for (auto [key, field] : kExtraErrorFields) {
    auto* msg = PQresultErrorField(handle_.get(), field);
    if (msg) {
      log_extra.Extend(key, std::string{msg});
    }
  }
  return log_extra;
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
