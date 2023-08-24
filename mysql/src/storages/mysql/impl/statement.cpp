#include <storages/mysql/impl/statement.hpp>

#include <utility>

#include <fmt/format.h>

#include <userver/logging/log.hpp>
#include <userver/tracing/scope_time.hpp>

#include <storages/mysql/impl/bindings/input_bindings.hpp>
#include <storages/mysql/impl/bindings/output_bindings.hpp>
#include <storages/mysql/impl/connection.hpp>
#include <storages/mysql/impl/native_interface.hpp>
#include <userver/storages/mysql/exceptions.hpp>
#include <userver/storages/mysql/impl/io/extractor.hpp>
#include <userver/storages/mysql/impl/io/params_binder_base.hpp>
#include <userver/storages/mysql/impl/tracing_tags.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

Statement::NativeStatementDeleter::NativeStatementDeleter(
    Connection* connection)
    : connection_{connection} {
  UASSERT(connection_);
}

void Statement::NativeStatementDeleter::operator()(MYSQL_STMT* statement) const
    noexcept {
  // This means deadline was set
  UASSERT(deadline_.IsReachable());
  try {
    NativeInterface{connection_->GetSocket(), deadline_}.StatementClose(
        statement);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "Failed to correctly dispose a prepared statement, it "
                     "might get leaked server-side. Error: "
                  << ex.what();
  }
}

void Statement::NativeStatementDeleter::SetDeadline(engine::Deadline deadline) {
  deadline_ = deadline;
}

Statement::Statement(Connection& connection, const std::string& statement,
                     engine::Deadline deadline)
    : statement_{statement},
      connection_{&connection},
      native_statement_{CreateStatement(deadline)} {}

Statement::~Statement() = default;

Statement::Statement(Statement&& other) noexcept = default;

StatementFetcher Statement::Execute(io::ParamsBinderBase& params,
                                    engine::Deadline deadline) {
  const USERVER_NAMESPACE::tracing::ScopeTime execute{
      impl::tracing::kExecuteScope};
  UpdateParamsBindings(params);

  const int err =
      NativeInterface{connection_->GetSocket(), deadline}.StatementExecute(
          native_statement_.get());

  if (err != 0) {
    throw MySQLStatementException{
        mysql_stmt_errno(native_statement_.get()),
        GetNativeError("Failed to execute a prepared statement")};
  }

  return StatementFetcher{*this};
}

void Statement::StoreResult(engine::Deadline deadline) {
  const int err =
      NativeInterface{connection_->GetSocket(), deadline}.StatementStoreResult(
          native_statement_.get());

  if (err != 0) {
    if (err == MYSQL_NO_DATA) {
      return;
    }

    throw MySQLStatementException{
        mysql_stmt_errno(native_statement_.get()),
        GetNativeError("Failed to fetch a prepared statement result")};
  }
}

bool Statement::FetchResultRow(bindings::OutputBindings& binds,
                               bool apply_binds, engine::Deadline deadline) {
  if (!binds.Empty() && apply_binds) {
    if (mysql_stmt_bind_result(native_statement_.get(),
                               binds.GetBindsArray()) != 0) {
      throw MySQLStatementException{
          mysql_stmt_errno(native_statement_.get()),
          GetNativeError("Failed to perform result binding")};
    }
  }

  const int fetch_err =
      NativeInterface{connection_->GetSocket(), deadline}.StatementFetch(
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

      const int err =
          mysql_stmt_fetch_column(native_statement_.get(), &bind, i, 0);
      if (err != 0) {
        throw std::runtime_error{GetNativeError(
            "Failed to fetch a column from statement result row")};
      }
    }

    binds.AfterFetch(i);
  }

  return true;
}

void Statement::Reset(engine::Deadline deadline) {
  const my_bool err =
      NativeInterface{connection_->GetSocket(), deadline}.StatementFreeResult(
          native_statement_.get());

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
    // https://bugs.mysql.com/bug.php?id=109380 (login required).
    if (connection_->GetServerInfo().server_type ==
        metadata::ServerInfo::Type::kMySQL) {
      PrepareStatement(native_statement_, deadline);
    }
    batch_size_.reset();
  }
}

void Statement::UpdateParamsBindings(io::ParamsBinderBase& params) {
  auto& binds = params.GetBinds();
  binds.ValidateAgainstStatement(*native_statement_);

  if (!binds.Empty()) {
    if (mysql_stmt_bind_param(native_statement_.get(), binds.GetBindsArray())) {
      throw std::runtime_error("Failed to bind statements params");
    }

    if (auto rows_count = params.GetRowsCount(); rows_count > 1) {
      const auto& server_info = connection_->GetServerInfo();
      if (server_info.server_type != metadata::ServerInfo::Type::kMariaDB ||
          server_info.server_version < metadata::SemVer{10, 2, 6}) {
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

std::size_t Statement::RowsCount() const {
  return mysql_stmt_num_rows(native_statement_.get());
}

std::size_t Statement::ParamsCount() {
  return mysql_stmt_param_count(native_statement_.get());
}

std::size_t Statement::ResultColumnsCount() {
  return mysql_stmt_field_count(native_statement_.get());
}

void Statement::SetReadonlyCursor(std::size_t batch_size) {
  std::size_t cursor_type = CURSOR_TYPE_READ_ONLY;
  mysql_stmt_attr_set(native_statement_.get(), STMT_ATTR_CURSOR_TYPE,
                      &cursor_type);
  mysql_stmt_attr_set(native_statement_.get(), STMT_ATTR_PREFETCH_ROWS,
                      &batch_size);

  batch_size_.emplace(batch_size);
}

void Statement::SetNoCursor() {
  std::size_t cursor_type = CURSOR_TYPE_NO_CURSOR;
  mysql_stmt_attr_set(native_statement_.get(), STMT_ATTR_CURSOR_TYPE,
                      &cursor_type);
  std::size_t batch_size = 0;
  mysql_stmt_attr_set(native_statement_.get(), STMT_ATTR_PREFETCH_ROWS,
                      &batch_size);

  batch_size_.reset();
}

std::optional<std::size_t> Statement::GetBatchSize() const {
  return batch_size_;
}

void Statement::SetDestructionDeadline(engine::Deadline deadline) {
  native_statement_.get_deleter().SetDeadline(deadline);
}

const std::string& Statement::GetStatementText() const { return statement_; }

Statement::NativeStatementPtr Statement::CreateStatement(
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

void Statement::PrepareStatement(NativeStatementPtr& native_statement,
                                 engine::Deadline deadline) {
  native_statement.get_deleter().SetDeadline(deadline);

  const int err =
      NativeInterface{connection_->GetSocket(), deadline}.StatementPrepare(
          native_statement.get(), statement_.data(), statement_.length());

  if (err != 0) {
    throw MySQLStatementException{
        mysql_stmt_errno(native_statement.get()),
        GetNativeError("Failed to initialize a prepared statement")};
  }
}

std::string Statement::GetNativeError(std::string_view prefix) const {
  return fmt::format("{}: error({}) [{}] \"{}\"", prefix,
                     mysql_stmt_errno(native_statement_.get()),
                     mysql_stmt_sqlstate(native_statement_.get()),
                     mysql_stmt_error(native_statement_.get()));
}

StatementFetcher::StatementFetcher(Statement& statement)
    : statement_{&statement} {
  UASSERT(statement_->native_statement_);
}

StatementFetcher::~StatementFetcher() {
  if (statement_) {
    try {
      // Although stmt_free_result may do some I/O usually it doesn't.
      statement_->Reset(parent_statement_deadline_);
    } catch (const std::exception& ex) {
      // Have to notify a connection here, otherwise it might be reused (and
      // even statement might be reused) when it's in invalid state.
      statement_->connection_->NotifyBroken();
      LOG_ERROR() << "Failed to correctly cleanup a statement: " << ex.what();
    }
  }
}

StatementFetcher::StatementFetcher(StatementFetcher&& other) noexcept
    : parent_statement_deadline_{other.parent_statement_deadline_},
      binds_applied_{other.binds_applied_},
      statement_{std::exchange(other.statement_, nullptr)} {}

bool StatementFetcher::FetchResult(io::ExtractorBase& extractor) {
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
      // statement.
      // sizeof(MYSQL_BIND) = 112 in 64bit mode, and without this
      // trickery we would have to copy the whole array for each column with
      // mysql_stst_bind_result, and for say 10 columns and 10^6 rows that
      // accounts for 10 * 112 * 10^6 ~= 1Gb of memcpy (and 10^6 rows of 10 ints
      // is only about 40Mb)
      //
      // There's no API to access the field, but it surely isn't going anywhere,
      // so we access it directly.
      // https://jira.mariadb.org/browse/CONC-620
      extractor.UpdateBinds(statement_->native_statement_->bind);
    }

    return true;
  });
}

std::uint64_t StatementFetcher::RowsAffected() const {
  const auto rows_affected =
      mysql_stmt_affected_rows(statement_->native_statement_.get());

  // libmariadb return -1 as ulonglong
  if (rows_affected == std::numeric_limits<std::uint64_t>::max()) {
    LOG_WARNING()
        << "RowsAffected called on a statement that doesn't affect any rows";
    return 0;
  }

  return rows_affected;
}

std::uint64_t StatementFetcher::LastInsertId() const {
  return mysql_stmt_insert_id(statement_->native_statement_.get());
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
