#include <storages/mysql/impl/mysql_statement.hpp>

// TODO : drop
#include <iostream>

#include <utility>

#include <fmt/format.h>

#include <userver/logging/log.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/utils/scope_guard.hpp>

#include <storages/mysql/impl/bindings/input_bindings.hpp>
#include <storages/mysql/impl/bindings/output_bindings.hpp>
#include <storages/mysql/impl/mysql_connection.hpp>
#include <userver/storages/mysql/impl/io/extractor.hpp>
#include <userver/storages/mysql/impl/io/params_binder_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

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
    : statement_{statement},
      connection_{&connection},
      native_statement_{CreateStatement(deadline)} {}

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

  if (batch_size_.has_value()) {
    // MySQL 8.0.31 seems to be buggy:
    // it doesn't send EOF_PACKET if we execute a statement without cursor
    // after we executed with cursor before, so we re-prepare the statement
    // https://bugs.mysql.com/bug.php?id=109380
    // TODO : conditionally on server version?
    if (connection_->GetServerInfo().server_type ==
        metadata::MySQLServerInfo::Type::kMySQL) {
      PrepareStatement(native_statement_, deadline);
    }
    batch_size_.reset();
  }
}

void MySQLStatement::UpdateParamsBindings(io::ParamsBinderBase& params) {
  auto& binds = params.GetBinds();
  binds.ValidateAgainstStatement(*native_statement_);

  if (!binds.Empty()) {
    if (mysql_stmt_bind_param(native_statement_.get(), binds.GetBindsArray())) {
      int a = 5;
    }

    if (auto rows_count = params.GetRowsCount(); rows_count > 1) {
      const auto& server_info = connection_->GetServerInfo();
      if (server_info.server_type !=
              metadata::MySQLServerInfo::Type::kMariaDB ||
          server_info.server_version < metadata::MySQLSemVer{10, 2, 0}) {
        throw std::logic_error{"Batch insert requires MariaDB 10.2 or later"};
      }

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

void MySQLStatement::SetReadonlyCursor(std::size_t batch_size) {
  std::size_t cursor_type = CURSOR_TYPE_READ_ONLY;
  mysql_stmt_attr_set(native_statement_.get(), STMT_ATTR_CURSOR_TYPE,
                      &cursor_type);
  mysql_stmt_attr_set(native_statement_.get(), STMT_ATTR_PREFETCH_ROWS,
                      &batch_size);

  batch_size_.emplace(batch_size);
}

void MySQLStatement::SetNoCursor() {
  std::size_t cursor_type = CURSOR_TYPE_NO_CURSOR;
  mysql_stmt_attr_set(native_statement_.get(), STMT_ATTR_CURSOR_TYPE,
                      &cursor_type);
  std::size_t batch_size = 0;
  mysql_stmt_attr_set(native_statement_.get(), STMT_ATTR_PREFETCH_ROWS,
                      &batch_size);

  batch_size_.reset();
}

std::optional<std::size_t> MySQLStatement::GetBatchSize() const {
  return batch_size_;
}

MySQLStatement::NativeStatementPtr MySQLStatement::CreateStatement(
    engine::Deadline deadline) {
  NativeStatementPtr statement_ptr{
      mysql_stmt_init(&connection_->GetNativeHandler()),
      NativeStatementDeleter(connection_)};
  if (!statement_ptr) {
    throw std::runtime_error{"Failed to initialize a prepared statement"};
  }

  PrepareStatement(statement_ptr, deadline);

  return statement_ptr;
}

void MySQLStatement::PrepareStatement(NativeStatementPtr& native_statement,
                                      engine::Deadline deadline) {
  int err = 0;
  connection_->GetSocket().RunToCompletion(
      [this, &err, &native_statement] {
        return mysql_stmt_prepare_start(&err, native_statement.get(),
                                        statement_.data(), statement_.length());
      },
      [&err, &native_statement](int mysql_events) {
        return mysql_stmt_prepare_cont(&err, native_statement.get(),
                                       mysql_events);
      },
      deadline);
  if (err != 0) {
    throw std::runtime_error{
        GetNativeError("Failed to initialize a prepared statement")};
  }
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
      statement_->Reset(parent_statement_deadline_);
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Failed to correctly cleanup a statement: " << ex.what();
    }
  }
}

MySQLStatementFetcher::MySQLStatementFetcher(
    MySQLStatementFetcher&& other) noexcept
    : parent_statement_deadline_{other.parent_statement_deadline_},
      binds_validated_{other.binds_validated_},
      statement_{std::exchange(other.statement_, nullptr)} {}

bool MySQLStatementFetcher::FetchResult(io::ExtractorBase& extractor) {
  auto broken_guard = statement_->connection_->GetBrokenGuard();

  const auto batch_size = statement_->GetBatchSize();

  if (!batch_size.has_value()) {
    statement_->StoreResult(parent_statement_deadline_);
  }

  const auto rows_count = batch_size.value_or(statement_->RowsCount());
  extractor.Reserve(rows_count);

  for (size_t i = 0; i < rows_count; ++i) {
    auto& binds = extractor.BindNextRow();
    if (!std::exchange(binds_validated_, true)) {
      binds.ValidateAgainstStatement(*statement_->native_statement_);
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

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
