#include <storages/mysql/impl/mysql_statement.hpp>

#include <userver/logging/log.hpp>

#include <storages/mysql/impl/mysql_binds_storage.hpp>
#include <storages/mysql/impl/mysql_connection.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace {

std::string GetStatementError(std::string_view prefix,
                              MYSQL_STMT* native_statement) {
  return std::string{prefix} + mysql_stmt_error(native_statement);
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

void MySQLStatement::BindStatementParams(BindsStorage& binds) {
  if (!binds.Empty()) {
    mysql_stmt_bind_param(native_statement_.get(), binds.GetBindsArray());
  }
}

void MySQLStatement::Execute(engine::Deadline deadline) {
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
    throw std::runtime_error{GetStatementError(
        "Failed to execute a prepared statement: ", native_statement_.get())};
  }
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
        GetStatementError("Failed to fetch a prepared statement result: ",
                          native_statement_.get())};
  }
}

bool MySQLStatement::FetchResultRow(BindsStorage& binds) {
  if (!binds.Empty()) {
    if (mysql_stmt_bind_result(native_statement_.get(),
                               binds.GetBindsArray()) != 0) {
      throw std::runtime_error{"Binding is messed up, TODO"};
    }
  }

  int fetch_error = mysql_stmt_fetch(native_statement_.get());
  if (fetch_error != 0) {
    if (fetch_error == MYSQL_NO_DATA) {
      return false;
    }

    if (fetch_error != MYSQL_DATA_TRUNCATED) {
      throw std::runtime_error{"No idea what this is"};
    }
  }

  const auto fields_count = mysql_stmt_field_count(native_statement_.get());
  auto* binds_array = binds.GetBindsArray();

  for (std::size_t i = 0; i < fields_count; ++i) {
    auto& bind = binds_array[i];
    if (bind.buffer_type == MYSQL_TYPE_STRING) {
      binds.ResizeOutputString(i);
    }

    int err = mysql_stmt_fetch_column(native_statement_.get(), &bind, i, 0);
    if (err != 0) {
      throw std::runtime_error{"Idk what this error actually is"};
    }
  }

  return true;
}

void MySQLStatement::FreeResult(engine::Deadline deadline) {
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
        GetStatementError("Failed to free a prepared statement result: ",
                          native_statement_.get())};
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
    throw std::runtime_error{GetStatementError(
        "Failed to initialize a prepared statement: ", statement_ptr.get())};
  }

  return statement_ptr;
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
