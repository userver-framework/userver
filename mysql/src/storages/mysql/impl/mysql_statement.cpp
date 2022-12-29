#include <storages/mysql/impl/mysql_statement.hpp>

#include <utility>

#include <fmt/format.h>

#include <userver/logging/log.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/utils/scope_guard.hpp>

#include <storages/mysql/impl/bindings/input_bindings.hpp>
#include <storages/mysql/impl/bindings/output_bindings.hpp>
#include <storages/mysql/impl/mysql_connection.hpp>
#include <storages/mysql/impl/mysql_native_interface.hpp>
#include <userver/storages/mysql/exceptions.hpp>
#include <userver/storages/mysql/impl/io/extractor.hpp>
#include <userver/storages/mysql/impl/io/params_binder_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

MySQLStatement::NativeStatementDeleter::NativeStatementDeleter(
    MySQLConnection* connection)
    : connection_{connection} {}

void MySQLStatement::NativeStatementDeleter::operator()(MYSQL_STMT* statement) {
  // This means deadline was set
  UASSERT(deadline_.IsReachable());
  try {
    MySQLNativeInterface{connection_->GetSocket(), deadline_}.StatementClose(
        statement);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "Failed to correctly dispose a prepared statement, it "
                     "might get leaked server-side. Error: "
                  << ex.what();
  }
}

void MySQLStatement::NativeStatementDeleter::SetDeadline(
    engine::Deadline deadline) {
  deadline_ = deadline;
}

MySQLStatement::MySQLStatement(MySQLConnection& connection,
                               const std::string& statement,
                               engine::Deadline deadline)
    : statement_{statement},
      connection_{&connection},
      native_statement_{CreateStatement(deadline)} {}

MySQLStatement::~MySQLStatement() = default;

MySQLStatement::MySQLStatement(MySQLStatement&& other) noexcept = default;

MySQLStatementFetcher MySQLStatement::Execute(io::ParamsBinderBase& params,
                                              engine::Deadline deadline) {
  tracing::ScopeTime execute{"execute"};
  UpdateParamsBindings(params);

  int err =
      MySQLNativeInterface{connection_->GetSocket(), deadline}.StatementExecute(
          native_statement_.get());

  if (err != 0) {
    throw MySQLStatementException{
        mysql_stmt_errno(native_statement_.get()),
        GetNativeError("Failed to execute a prepared statement")};
  }

  return MySQLStatementFetcher{*this};
}

void MySQLStatement::StoreResult(engine::Deadline deadline) {
  int err = MySQLNativeInterface{connection_->GetSocket(), deadline}
                .StatementStoreResult(native_statement_.get());

  if (err != 0) {
    if (err == MYSQL_NO_DATA) {
      return;
    }

    throw MySQLStatementException{
        mysql_stmt_errno(native_statement_.get()),
        GetNativeError("Failed to fetch a prepared statement result")};
  }
}

bool MySQLStatement::FetchResultRow(bindings::OutputBindings& binds,
                                    bool apply_binds,
                                    engine::Deadline deadline) {
  if (!binds.Empty() && apply_binds) {
    if (mysql_stmt_bind_result(native_statement_.get(),
                               binds.GetBindsArray()) != 0) {
      throw MySQLStatementException{
          mysql_stmt_errno(native_statement_.get()),
          GetNativeError("Failed to perform result binding")};
    }
  }

  int fetch_err =
      MySQLNativeInterface{connection_->GetSocket(), deadline}.StatementFetch(
          native_statement_.get());

  if (fetch_err != 0) {
    if (fetch_err == MYSQL_NO_DATA) {
      return false;
    }

    if (fetch_err != MYSQL_DATA_TRUNCATED) {
      // MYSQL_DATA_TRUNCATED is what we get with the way we bind strings,
      // it's ok. Other errors are not ok tho.
      throw MySQLStatementException{
          mysql_stmt_errno(native_statement_.get()),
          GetNativeError("Failed to fetch statement result row")};
    }
  }

  auto* binds_array = binds.GetBindsArray();

  for (std::size_t i = 0; i < binds.Size(); ++i) {
    binds.BeforeFetch(i);

    auto& bind = binds_array[i];
    // We don't have to fetch again if there was no truncation. Not refetching
    // gives ~30% speedup in benchmarks
    if (bind.error_value) {
      UASSERT(fetch_err == MYSQL_DATA_TRUNCATED);

      int err = mysql_stmt_fetch_column(native_statement_.get(), &bind, i, 0);
      if (err != 0) {
        throw std::runtime_error{GetNativeError(
            "Failed to fetch a column from statement result row")};
      }
    }

    binds.AfterFetch(i);
  }

  return true;
}

void MySQLStatement::Reset(engine::Deadline deadline) {
  my_bool err = MySQLNativeInterface{connection_->GetSocket(), deadline}
                    .StatementFreeResult(native_statement_.get());

  if (err != 0) {
    throw MySQLStatementException{
        mysql_stmt_errno(native_statement_.get()),
        GetNativeError("Failed to free a prepared statement result")};
  }

  // TODO : we probably need to reset the statement when cursor processing
  // fails. Apart from that - not sure if that's ever needed.
  // Actually mysql_stmt_free_result closes any open cursors, idk.

  if (batch_size_.has_value()) {
    // MySQL 8.0.31 seems to be buggy:
    // it doesn't send EOF_PACKET if we execute a statement without cursor
    // after we executed with cursor before, so we re-prepare the statement
    // https://bugs.mysql.com/bug.php?id=109380
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
      throw std::runtime_error("Failed to bind statements params");
    }

    if (auto rows_count = params.GetRowsCount(); rows_count > 1) {
      const auto& server_info = connection_->GetServerInfo();
      if (server_info.server_type !=
              metadata::MySQLServerInfo::Type::kMariaDB ||
          server_info.server_version < metadata::MySQLSemVer{10, 2, 6}) {
        throw std::logic_error{"Batch insert requires MariaDB 10.2.6 or later"};
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

void MySQLStatement::SetDestructionDeadline(engine::Deadline deadline) {
  native_statement_.get_deleter().SetDeadline(deadline);
}

MySQLStatement::NativeStatementPtr MySQLStatement::CreateStatement(
    engine::Deadline deadline) {
  NativeStatementPtr statement_ptr{
      mysql_stmt_init(&connection_->GetNativeHandler()),
      NativeStatementDeleter(connection_)};
  if (!statement_ptr) {
    throw MySQLStatementException{
        mysql_errno(&connection_->GetNativeHandler()),
        connection_->GetNativeError(
            "Failed to initialize a prepared statement")};
  }

  PrepareStatement(statement_ptr, deadline);

  return statement_ptr;
}

void MySQLStatement::PrepareStatement(NativeStatementPtr& native_statement,
                                      engine::Deadline deadline) {
  int err =
      MySQLNativeInterface{connection_->GetSocket(), deadline}.StatementPrepare(
          native_statement.get(), statement_.data(), statement_.length());

  if (err != 0) {
    throw MySQLStatementException{
        mysql_stmt_errno(native_statement.get()),
        GetNativeError("Failed to initialize a prepared statement")};
  }
}

std::string MySQLStatement::GetNativeError(std::string_view prefix) const {
  return fmt::format("{}: error({}) [{}] \"{}\"", prefix,
                     mysql_stmt_errno(native_statement_.get()),
                     mysql_stmt_sqlstate(native_statement_.get()),
                     mysql_stmt_error(native_statement_.get()));
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
      // Have to notify a connection here, otherwise it might be reused (and
      // even statement might be reused) when it's in invalid state.
      statement_->connection_->NotifyBroken();
      LOG_ERROR() << "Failed to correctly cleanup a statement: " << ex.what();
    }
  }
}

MySQLStatementFetcher::MySQLStatementFetcher(
    MySQLStatementFetcher&& other) noexcept
    : parent_statement_deadline_{other.parent_statement_deadline_},
      binds_applied_{other.binds_applied_},
      statement_{std::exchange(other.statement_, nullptr)} {}

bool MySQLStatementFetcher::FetchResult(io::ExtractorBase& extractor) {
  auto guard = statement_->connection_->GetBrokenGuard();

  return guard.Execute([&] {
    const auto batch_size = statement_->GetBatchSize();

    if (!batch_size.has_value()) {
      statement_->StoreResult(parent_statement_deadline_);
    }

    const auto rows_count = batch_size.value_or(statement_->RowsCount());
    extractor.Reserve(rows_count);

    const auto validate_binds = !std::exchange(binds_validated_, true);
    if (validate_binds) {
      // We validate binds even for empty results:
      // we don't want a query returning empty result to pass tests and then
      // BOOM with actual data. Performance cost of this should be negligible,
      // if any.
      auto& binds = extractor.BindNextRow();
      binds.ValidateAgainstStatement(*statement_->native_statement_);
      extractor.RollbackLastRow();
    }

    for (size_t i = 0; i < rows_count; ++i) {
      auto& binds = extractor.BindNextRow();

      const auto apply_binds = !std::exchange(binds_applied_, true);
      // We don't have to reapply binds all the time: values in them are changed
      // by extractor.BindNextRow() call, and mysql_stmt_bind_result is costly.
      // Not reapplying them for each row gives ~20% speedup in benchmarks with
      // wide select.
      const auto parsed = statement_->FetchResultRow(
          binds, apply_binds, parent_statement_deadline_);
      if (!parsed) {
        extractor.RollbackLastRow();
        return false;
      }
      extractor.CommitLastRow();
      // With this we make OutputBinder operate on MYSQL_BIND array stored in
      // statement. sizeof(MYSQL_BIND) = 112 in 64bit mode, and without this
      // trickery we would have to copy the whole array for each column with
      // mysql_stst_bind_result, and for say 10 columns and 10^6 rows that
      // accounts for 10 * 112 * 10^6 ~= 1Gb of memcpy (and 10^6 rows of 10 ints
      // is only about 40Mb)
      extractor.UpdateBinds(statement_->native_statement_->bind);
    }

    return true;
  });
}

std::size_t MySQLStatementFetcher::RowsAffected() const {
  const auto rows_affected =
      mysql_stmt_affected_rows(statement_->native_statement_.get());

  // libmariadb return -1 as ulonglong
  if (rows_affected == std::numeric_limits<std::size_t>::max()) {
    LOG_WARNING()
        << "RowsAffected called on a statement that doesn't affect any rows";
    return 0;
  }

  return rows_affected;
}

std::size_t MySQLStatementFetcher::LastInsertId() const {
  return mysql_stmt_insert_id(statement_->native_statement_.get());
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
