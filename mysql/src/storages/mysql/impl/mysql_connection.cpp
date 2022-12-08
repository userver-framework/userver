#include <storages/mysql/impl/mysql_connection.hpp>

#include <stdexcept>

#include <fmt/format.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/scope_guard.hpp>

#include <storages/mysql/impl/mysql_binds_storage.hpp>
#include <storages/mysql/impl/mysql_plain_query.hpp>
#include <userver/storages/mysql/io/extractor.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace {

MYSQL InitMysql() {
  MYSQL mysql;
  mysql_init(&mysql);
  mysql_options(&mysql, MYSQL_OPT_NONBLOCK, nullptr);

  return mysql;
}

}  // namespace

MySQLConnection::MySQLConnection()
    : mysql_{InitMysql()}, socket_{InitSocket()} {
  // TODO : deadline and cleanup
  while (socket_.ShouldWait()) {
    const auto mysql_events = socket_.Wait({});
    socket_.SetEvents(
        mysql_real_connect_cont(&connect_ret_, &mysql_, mysql_events));
  }

  if (!connect_ret_) {
    throw std::runtime_error{std::string{"Failed to connect: "} +
                             mysql_error(&mysql_)};
  }
}

MySQLConnection::~MySQLConnection() {
  socket_.RunToCompletion([this] { return mysql_close_start(&mysql_); },
                          [this](int mysql_event) {
                            return mysql_close_cont(&mysql_, mysql_event);
                          },
                          {} /* TODO : deadline */);
}

MySQLResult MySQLConnection::ExecutePlain(const std::string& query,
                                          engine::Deadline deadline) {
  MySQLPlainQuery mysql_query{*this, query};

  mysql_query.Execute(deadline);
  return mysql_query.FetchResult(deadline);
}

void MySQLConnection::ExecuteStatement(const std::string& statement,
                                       io::ParamsBinderBase& params,
                                       io::ExtractorBase& extractor,
                                       engine::Deadline deadline) {
  auto& mysql_statement = PrepareStatement(statement, deadline);

  UINVARIANT(
      mysql_statement.ParamsCount() == params.GetBinds().Size(),
      fmt::format("Statement has {} parameters, but {} were provided",
                  mysql_statement.ParamsCount(), params.GetBinds().Size()));
  UINVARIANT(
      mysql_statement.ResultColumnsCount() == extractor.ColumnsCount(),
      fmt::format("Statement has {} output columns, but {} were provided",
                  mysql_statement.ResultColumnsCount(),
                  extractor.ColumnsCount()));

  mysql_statement.BindStatementParams(params.GetBinds());
  mysql_statement.Execute(deadline);

  utils::ScopeGuard result_scope_dispose{
      [&mysql_statement, deadline] { mysql_statement.FreeResult(deadline); }};

  mysql_statement.StoreResult(deadline);

  const auto rows_count = mysql_statement.RowsCount();
  extractor.Reserve(rows_count);

  for (std::size_t i = 0; i < rows_count; ++i) {
    auto& bind = extractor.BindNextRow();
    const auto parsed = mysql_statement.FetchResultRow(bind);
    UASSERT(parsed);
  }
}

MySQLSocket& MySQLConnection::GetSocket() { return socket_; }

MYSQL& MySQLConnection::GetNativeHandler() { return mysql_; }

const char* MySQLConnection::GetNativeError() { return mysql_error(&mysql_); }

std::string MySQLConnection::GetNativeError(std::string_view prefix) {
  return std::string{prefix} + mysql_error(&mysql_);
}

MySQLSocket MySQLConnection::InitSocket() {
  const auto mysql_events = mysql_real_connect_start(
      &connect_ret_, &mysql_, "0.0.0.0", "test", "test", "test", 0 /* port */,
      nullptr /* unix_socket */, 0 /* client_flags */);

  return MySQLSocket{mysql_get_socket(&mysql_), mysql_events};
}

MySQLStatement& MySQLConnection::PrepareStatement(const std::string& statement,
                                                  engine::Deadline deadline) {
  // TODO : case insensitive find
  if (auto it = statements_cache_.find(statement);
      it != statements_cache_.end()) {
    return it->second;
  }

  MySQLStatement mysql_statement{*this, statement, deadline};
  auto [it, inserted] =
      statements_cache_.emplace(statement, std::move(mysql_statement));
  UASSERT(inserted);
  return it->second;
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
