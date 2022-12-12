#include <storages/mysql/impl/mysql_statement.hpp>

#include <utility>

#include <fmt/format.h>

#include <userver/logging/log.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/utils/scope_guard.hpp>

#include <storages/mysql/impl/bindings/input_bindings.hpp>
#include <storages/mysql/impl/bindings/output_bindings.hpp>
#include <storages/mysql/impl/mysql_connection.hpp>
#include <userver/storages/mysql/io/binder.hpp>
#include <userver/storages/mysql/io/extractor.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace {

const char* GetFieldTypeName(enum_field_types type) {
  switch (type) {
    case MYSQL_TYPE_TINY:
      return "MYSQL_TYPE_TINY";
    case MYSQL_TYPE_SHORT:
      return "MYSQL_TYPE_SHORT";
    case MYSQL_TYPE_LONG:
      return "MYSQL_TYPE_LONG";
    case MYSQL_TYPE_LONGLONG:
      return "MYSQL_TYPE_LONGLONG";
    case MYSQL_TYPE_FLOAT:
      return "MYSQL_TYPE_FLOAT";
    case MYSQL_TYPE_DOUBLE:
      return "MYSQL_TYPE_DOUBLE";
    case MYSQL_TYPE_STRING:
      return "MYSQL_TYPE_STRING";
    case MYSQL_TYPE_VAR_STRING:
      return "MYSQL_TYPE_VAR_STRING";
    case MYSQL_TYPE_BLOB:
      return "MYSQL_TYPE_BLOB";

    default:
      return "unmapped";
  }
}

bool IsBindable(enum_field_types bind_type, enum_field_types field_type) {
  if (bind_type == MYSQL_TYPE_STRING) {
    return field_type == MYSQL_TYPE_VARCHAR ||
           field_type == MYSQL_TYPE_VAR_STRING || field_type == MYSQL_TYPE_BLOB;
  }

  return bind_type == field_type;
}

void ValidateTypesMatch(MYSQL_STMT* statement, MYSQL_BIND* binds,
                        std::size_t fields_count) {
  // TODO : fix for nulls
  return;
  auto* fields = statement->fields;

  for (std::size_t i = 0; i < fields_count; ++i) {
    auto& bind = binds[i];
    auto& field = fields[i];

    UINVARIANT(IsBindable(bind.buffer_type, field.type),
               fmt::format("Type mismatch for field '{}' ({}-th field): "
                           "expected '{}', got '{}'",
                           field.name, i, GetFieldTypeName(field.type),
                           GetFieldTypeName(bind.buffer_type)));

    const auto signed_to_string = [](bool is_signed) -> std::string_view {
      return is_signed ? "signed" : "unsigned";
    };
    UINVARIANT(bind.is_unsigned == ((field.flags & UNSIGNED_FLAG) != 0),
               fmt::format("Type signed/unsigned mismatch for field '{}' "
                           "({}-th field): {} in DB, {} in user type",
                           field.name, i,
                           signed_to_string((field.flags & UNSIGNED_FLAG) != 0),
                           signed_to_string(bind.is_unsigned)));

    // TODO : nullable
  }
}

}  // namespace

MySQLStatement::NativeStatementDeleter::NativeStatementDeleter(
    MySQLConnection* connection)
    : connection_{connection} {}

void MySQLStatement::NativeStatementDeleter::operator()(MYSQL_STMT* statement) {
  try {
    my_bool err{};
    connection_->GetSocket().RunToCompletion(
        [&err, statement] { return mysql_stmt_close_start(&err, statement); },
        [&err, statement](int mysql_events) {
          return mysql_stmt_close_cont(&err, statement, mysql_events);
        },
        {} /* TODO : deadline */);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "Failed to correctly dispose a prepared statement, it "
                     "might get leaked server-side. Error: "
                  << ex.what();
  }
}

MySQLStatement::MySQLStatement(MySQLConnection& connection,
                               const std::string& statement,
                               engine::Deadline deadline)
    : connection_{&connection},
      native_statement_{CreateStatement(statement, deadline)} {}

MySQLStatement::~MySQLStatement() = default;

MySQLStatement::MySQLStatement(MySQLStatement&& other) noexcept = default;

MySQLStatementFetcher MySQLStatement::Execute(engine::Deadline deadline,
                                              io::ParamsBinderBase& params) {
  tracing::ScopeTime execute{"execute"};
  UpdateParamsBindings(params);

  int err = 0;
  connection_->GetSocket().RunToCompletion(
      [this, &err] {
        return mysql_stmt_execute_start(&err, native_statement_.get());
      },
      [this, &err](int mysql_events) {
        return mysql_stmt_execute_cont(&err, native_statement_.get(),
                                       mysql_events);
      },
      deadline);

  if (err != 0) {
    throw std::runtime_error{
        GetNativeError("Failed to execute a prepared statement")};
  }

  return MySQLStatementFetcher{*this};
}

void MySQLStatement::StoreResult(engine::Deadline deadline) {
  int err = 0;
  connection_->GetSocket().RunToCompletion(
      [this, &err] {
        return mysql_stmt_store_result_start(&err, native_statement_.get());
      },
      [this, &err](int mysql_events) {
        return mysql_stmt_store_result_cont(&err, native_statement_.get(),
                                            mysql_events);
      },
      deadline);

  if (err != 0) {
    if (err == MYSQL_NO_DATA) {
      return;
    }

    throw std::runtime_error{
        GetNativeError("Failed to fetch a prepared statement result")};
  }
}

bool MySQLStatement::FetchResultRow(bindings::OutputBindings& binds,
                                    engine::Deadline deadline) {
  if (!binds.Empty()) {
    if (mysql_stmt_bind_result(native_statement_.get(),
                               binds.GetBindsArray()) != 0) {
      throw std::runtime_error{
          GetNativeError("Failed to perform result binding")};
    }
  }

  int fetch_err = 0;
  connection_->GetSocket().RunToCompletion(
      [this, &fetch_err] {
        return mysql_stmt_fetch_start(&fetch_err, native_statement_.get());
      },
      [this, &fetch_err](int mysql_events) {
        return mysql_stmt_fetch_cont(&fetch_err, native_statement_.get(),
                                     mysql_events);
      },
      deadline);

  if (fetch_err != 0) {
    if (fetch_err == MYSQL_NO_DATA) {
      return false;
    }

    if (fetch_err != MYSQL_DATA_TRUNCATED) {
      // MYSQL_DATA_TRUNCATED is what we get with the way we bind strings,
      // it's ok. Other errors are not ok tho.
      throw std::runtime_error{
          GetNativeError("Failed to fetch statement result row")};
    }
  }

  auto* binds_array = binds.GetBindsArray();

  for (std::size_t i = 0; i < binds.Size(); ++i) {
    binds.BeforeFetch(i);

    auto& bind = binds_array[i];
    int err = mysql_stmt_fetch_column(native_statement_.get(), &bind, i, 0);
    if (err != 0) {
      throw std::runtime_error{
          GetNativeError("Failed to fetch a column from statement result row")};
    }

    binds.AfterFetch(i);
  }

  return true;
}

void MySQLStatement::Reset(engine::Deadline deadline) {
  my_bool err{};
  connection_->GetSocket().RunToCompletion(
      [this, &err] {
        return mysql_stmt_free_result_start(&err, native_statement_.get());
      },
      [this, &err](int mysql_events) {
        return mysql_stmt_free_result_cont(&err, native_statement_.get(),
                                           mysql_events);
      },
      deadline);

  if (err != 0) {
    throw std::runtime_error{
        GetNativeError("Failed to free a prepared statement result")};
  }

  connection_->GetSocket().RunToCompletion(
      [this, &err] {
        return mysql_stmt_reset_start(&err, native_statement_.get());
      },
      [this, &err](int mysql_events) {
        return mysql_stmt_reset_cont(&err, native_statement_.get(),
                                     mysql_events);
      },
      deadline);
  if (err != 0) {
    throw std::runtime_error{
        GetNativeError("Failed to reset a prepared statement")};
  }
}

void MySQLStatement::UpdateParamsBindings(io::ParamsBinderBase& params) {
  auto& binds = params.GetBinds();

  UINVARIANT(ParamsCount() == binds.Size(),
             fmt::format("Statement has {} parameters, but {} were provided",
                         ParamsCount(), binds.Size()));
  if (!binds.Empty()) {
    mysql_stmt_bind_param(native_statement_.get(), binds.GetBindsArray());

    if (auto rows_count = params.GetRowsCount(); rows_count > 1) {
      mysql_stmt_attr_set(native_statement_.get(), STMT_ATTR_ARRAY_SIZE,
                          &rows_count);
    }

    if (auto* user_data = binds.GetUserData()) {
      mysql_stmt_attr_set(native_statement_.get(), STMT_ATTR_CB_USER_DATA,
                          user_data);
    }
    if (auto* param_cb = binds.GetParamsCallback()) {
      mysql_stmt_attr_set(native_statement_.get(), STMT_ATTR_CB_PARAM,
                          reinterpret_cast<void*>(param_cb));
    }
  }
}

std::size_t MySQLStatement::RowsCount() const {
  return mysql_stmt_num_rows(native_statement_.get());
}

std::size_t MySQLStatement::ParamsCount() {
  return mysql_stmt_param_count(native_statement_.get());
}

std::size_t MySQLStatement::ResultColumnsCount() {
  return mysql_stmt_field_count(native_statement_.get());
}

MySQLStatement::NativeStatementPtr MySQLStatement::CreateStatement(
    const std::string& statement, engine::Deadline deadline) {
  NativeStatementPtr statement_ptr{
      mysql_stmt_init(&connection_->GetNativeHandler()),
      NativeStatementDeleter(connection_)};
  if (!statement_ptr) {
    throw std::runtime_error{"Failed to initialize a prepared statement"};
  }

  int err = 0;
  connection_->GetSocket().RunToCompletion(
      [&err, &statement, &statement_ptr] {
        return mysql_stmt_prepare_start(&err, statement_ptr.get(),
                                        statement.data(), statement.length());
      },
      [&err, &statement_ptr](int mysql_events) {
        return mysql_stmt_prepare_cont(&err, statement_ptr.get(), mysql_events);
      },
      deadline);
  if (err != 0) {
    throw std::runtime_error{
        GetNativeError("Failed to initialize a prepared statement")};
  }

  // We force statements to return a cursor, because otherwise Execute returns a
  // whole result set, which is not always desirable (we support async iteration
  // via AsStreamOf)
  std::size_t cursor_type = CURSOR_TYPE_READ_ONLY;
  mysql_stmt_attr_set(native_statement_.get(), STMT_ATTR_CURSOR_TYPE,
                      &cursor_type);

  return statement_ptr;
}

std::string MySQLStatement::GetNativeError(std::string_view prefix) const {
  return fmt::format("{}: {}. Errno: {}", prefix,
                     mysql_stmt_error(native_statement_.get()),
                     mysql_stmt_errno(native_statement_.get()));
}

MySQLStatementFetcher::MySQLStatementFetcher(MySQLStatement& statement)
    : statement_{&statement} {
  UASSERT(statement_->native_statement_);
}

MySQLStatementFetcher::~MySQLStatementFetcher() {
  if (statement_) {
    try {
      // TODO : mysql_stmt_reset ?
      statement_->Reset(parent_statement_deadline_);
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Failed to correctly cleanup a statement: " << ex.what();
    }
  }
}

MySQLStatementFetcher::MySQLStatementFetcher(
    MySQLStatementFetcher&& other) noexcept
    : parent_statement_deadline_{other.parent_statement_deadline_},
      batch_size_{other.batch_size_},
      binds_validated_{other.binds_validated_},
      statement_{std::exchange(other.statement_, nullptr)} {}

bool MySQLStatementFetcher::FetchResult(io::ExtractorBase& extractor) {
  UINVARIANT(
      statement_->ResultColumnsCount() == extractor.ColumnsCount(),
      fmt::format("Statement has {} output columns, but {} were provided",
                  statement_->ResultColumnsCount(), extractor.ColumnsCount()));
  auto broken_guard = statement_->connection_->GetBrokenGuard();

  if (!batch_size_.has_value()) {
    statement_->StoreResult(parent_statement_deadline_);
  }

  const auto rows_count = batch_size_.value_or(statement_->RowsCount());
  extractor.Reserve(rows_count);

  for (size_t i = 0; i < rows_count; ++i) {
    auto& binds = extractor.BindNextRow();
    if (!std::exchange(binds_validated_, true)) {
      ValidateBinding(binds);
    }

    const auto parsed =
        statement_->FetchResultRow(binds, parent_statement_deadline_);
    if (!parsed) {
      extractor.RollbackLastRow();
      return false;
    }
  }

  return true;
}

void MySQLStatementFetcher::SetFetchBatchSize(std::size_t batch_size) {
  batch_size_.emplace(batch_size);
  mysql_stmt_attr_set(statement_->native_statement_.get(),
                      STMT_ATTR_PREFETCH_ROWS, &batch_size);
}

void MySQLStatementFetcher::ValidateBinding(bindings::OutputBindings& binds) {
  const auto fields_count = statement_->ResultColumnsCount();
  UASSERT(binds.Size() == fields_count);

  ValidateTypesMatch(statement_->native_statement_.get(), binds.GetBindsArray(),
                     fields_count);
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
