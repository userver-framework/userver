#include <storages/mysql/impl/mysql_connection.hpp>

#include <stdexcept>

#include <fmt/format.h>

#include <userver/logging/log.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/utils/assert.hpp>
// TODO : drop
#include <userver/engine/sleep.hpp>

#include <storages/mysql/impl/mysql_plain_query.hpp>
#include <userver/storages/mysql/io/extractor.hpp>

#include <storages/mysql/settings/host_settings.hpp>

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

MySQLConnection::MySQLConnection(const settings::HostSettings& host_settings,
                                 engine::Deadline deadline)
    : host_settings_{host_settings},
      host_ip_{host_settings_.GetHostIp(deadline)},
      mysql_{InitMysql()},
      socket_{InitSocket()} {
  // TODO : deadline and cleanup
  while (socket_.ShouldWait()) {
    const auto mysql_events = socket_.Wait({});
    socket_.SetEvents(
        mysql_real_connect_cont(&connect_ret_, &mysql_, mysql_events));
  }

  if (!connect_ret_) {
    throw std::runtime_error{GetNativeError("Failed to connect")};
  }
}

MySQLConnection::~MySQLConnection() {
  try {
    socket_.RunToCompletion([this] { return mysql_close_start(&mysql_); },
                            [this](int mysql_event) {
                              return mysql_close_cont(&mysql_, mysql_event);
                            },
                            {} /* TODO : deadline */);
  } catch (const std::exception& ex) {
    LOG_WARNING() << "Failed to correctly release a connection: " << ex.what();
  }
}

MySQLResult MySQLConnection::ExecutePlain(const std::string& query,
                                          engine::Deadline deadline) {
  auto broken_guard = GetBrokenGuard();

  MySQLPlainQuery mysql_query{*this, query};
  mysql_query.Execute(deadline);
  return mysql_query.FetchResult(deadline);
}

MySQLStatementFetcher MySQLConnection::ExecuteStatement(
    const std::string& statement, io::ParamsBinderBase& params,
    engine::Deadline deadline) {
  auto broken_guard = GetBrokenGuard();

  auto& mysql_statement = PrepareStatement(statement, deadline);

  return mysql_statement.Execute(deadline, params);
}

void MySQLConnection::ExecuteInsert(const std::string& insert_statement,
                                    io::ParamsBinderBase& params,
                                    engine::Deadline deadline) {
  ExecuteStatement(insert_statement, params, deadline);
}

void MySQLConnection::Ping(engine::Deadline deadline) {
  auto broken_guard = GetBrokenGuard();

  int err = 0;
  socket_.RunToCompletion(
      [this, &err] { return mysql_ping_start(&err, &mysql_); },
      [this, &err](int mysql_events) {
        return mysql_ping_cont(&err, &mysql_, mysql_events);
      },
      deadline);

  if (err != 0) {
    throw std::runtime_error{GetNativeError("Failed to ping the server")};
  }
}

MySQLSocket& MySQLConnection::GetSocket() { return socket_; }

MYSQL& MySQLConnection::GetNativeHandler() { return mysql_; }

bool MySQLConnection::IsBroken() const { return broken_.load(); }

const char* MySQLConnection::GetNativeError() { return mysql_error(&mysql_); }

std::string MySQLConnection::GetNativeError(std::string_view prefix) {
  return fmt::format("{}: {}. Errno: {}", prefix, mysql_error(&mysql_),
                     mysql_errno(&mysql_));
}

std::string MySQLConnection::EscapeString(std::string_view source) {
  auto buffer = std::make_unique<char[]>(source.length() * 2 + 1);
  std::size_t escaped_length = mysql_real_escape_string(
      &mysql_, buffer.get(), source.data(), source.length());

  return std::string(buffer.get(), escaped_length);
}

MySQLConnection::BrokenGuard MySQLConnection::GetBrokenGuard() {
  return BrokenGuard{*this};
}

MySQLSocket MySQLConnection::InitSocket() {
  const auto& auth_settings = host_settings_.GetAuthSettings();

  const auto mysql_events = mysql_real_connect_start(
      &connect_ret_, &mysql_, host_ip_.c_str(), auth_settings.user.c_str(),
      auth_settings.password.c_str(), auth_settings.dbname.c_str(),
      host_settings_.GetPort(), nullptr /* unix_socket */,
      0 /* client_flags */);

  return MySQLSocket{mysql_get_socket(&mysql_), mysql_events};
}

MySQLStatement& MySQLConnection::PrepareStatement(const std::string& statement,
                                                  engine::Deadline deadline) {
  // TODO : case insensitive find
  if (auto it = statements_cache_.find(statement);
      it != statements_cache_.end()) {
    return it->second;
  }

  tracing::ScopeTime prepare{"prepare"};
  MySQLStatement mysql_statement{*this, statement, deadline};
  auto [it, inserted] =
      statements_cache_.emplace(statement, std::move(mysql_statement));
  UASSERT(inserted);
  return it->second;
}

MySQLConnection::BrokenGuard::BrokenGuard(MySQLConnection& connection)
    : exceptions_on_enter_{std::uncaught_exceptions()},
      broken_{connection.broken_} {
  if (broken_.load()) {
    throw std::runtime_error{"Connection is broken"};
  }
}

MySQLConnection::BrokenGuard::~BrokenGuard() {
  if (exceptions_on_enter_ != std::uncaught_exceptions()) {
    broken_.store(true);
  }
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
