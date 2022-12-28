#include <storages/mysql/impl/mysql_connection.hpp>

#include <stdexcept>

#include <fmt/format.h>

#include <userver/clients/dns/resolver.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/utils/assert.hpp>

#include <storages/mysql/impl/metadata/mysql_server_info.hpp>
#include <storages/mysql/impl/mysql_native_interface.hpp>
#include <storages/mysql/impl/mysql_plain_query.hpp>
#include <storages/mysql/settings/settings.hpp>
#include <userver/storages/mysql/exceptions.hpp>
#include <userver/storages/mysql/impl/io/extractor.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace {

// quoting the docs:
// "mysql_close() sends a COM_QUIT request to the server, though it does not
// wait for any reply. Thus, theoretically it can block (if the socket buffer
// is full), though in practise it is probably unlikely to occur frequently."
//
// If it blocks we use `shutdown` on the socket, because there is no other way
// to honor any kind of deadline, so this is zero.
constexpr std::chrono::milliseconds kDefaultCloseTimeout{0};

void SetNativeOptions(MYSQL* mysql,
                      const settings::ConnectionSettings& connection_settings) {
  // making everything async
  mysql_optionsv(mysql, MYSQL_OPT_NONBLOCK, nullptr);
  // no automatic reconnect
  my_bool reconnect = 0;
  mysql_optionsv(mysql, MYSQL_OPT_RECONNECT, &reconnect);
  // report MYSQL_DATA_TRUNCATED for prepared statements fetch
  mysql_optionsv(mysql, MYSQL_REPORT_DATA_TRUNCATION, "1");
  // set maximum packet length higher than default (16Mb), because why not
  std::size_t max_allowed_packet = 0x8000000;
  mysql_optionsv(mysql, MYSQL_OPT_MAX_ALLOWED_PACKET,
                 &max_allowed_packet);  // 128Mb
  // set socket buffer size higher than default(16Kb),
  // because 16Kb seems low to me
  std::size_t net_buffer_size = 0x10000;
  mysql_optionsv(mysql, MYSQL_OPT_NET_BUFFER_LENGTH, &net_buffer_size);  // 64Kb
  // force TCP just to be sure
  enum mysql_protocol_type prot_type = MYSQL_PROTOCOL_TCP;
  mysql_optionsv(mysql, MYSQL_OPT_PROTOCOL, &prot_type);
  // introduce ourselves
  mysql_optionsv(mysql, MYSQL_OPT_CONNECT_ATTR_ADD, "client_name",
                 "userver-mysql");

  if (connection_settings.use_compression) {
    mysql_optionsv(mysql, MYSQL_OPT_COMPRESS, nullptr);
  }
}

}  // namespace

MySQLConnection::MySQLConnection(
    clients::dns::Resolver& resolver,
    const settings::EndpointInfo& endpoint_info,
    const settings::AuthSettings& auth_settings,
    const settings::ConnectionSettings& connection_settings,
    engine::Deadline deadline)
    : socket_{-1, 0},
      statements_cache_{*this, connection_settings.statements_cache_size} {
  InitSocket(resolver, endpoint_info, auth_settings, connection_settings,
             deadline);
  server_info_ = metadata::MySQLServerInfo::Get(mysql_);

  const auto server_info = metadata::MySQLServerInfo::Get(mysql_);
  LOG_INFO() << "MySQL connection initialized."
             << " Server type: " << server_info.server_type_str << " "
             << server_info.server_version.ToString();
}

MySQLConnection::~MySQLConnection() {
  // We close the connection before resetting statements cache, so that reset
  // doesn't do potentially costly I/O
  Close(engine::Deadline::FromDuration(kDefaultCloseTimeout));
}

MySQLResult MySQLConnection::ExecutePlain(const std::string& query,
                                          engine::Deadline deadline) {
  auto guard = GetBrokenGuard();

  return guard.Execute([&] {
    MySQLPlainQuery mysql_query{*this, query};
    mysql_query.Execute(deadline);
    return mysql_query.FetchResult(deadline);
  });
}

MySQLStatementFetcher MySQLConnection::ExecuteStatement(
    const std::string& statement, io::ParamsBinderBase& params,
    engine::Deadline deadline, std::optional<std::size_t> batch_size) {
  auto guard = GetBrokenGuard();

  return guard.Execute([&] {
    auto& mysql_statement = PrepareStatement(statement, deadline, batch_size);

    return mysql_statement.Execute(params, deadline);
  });
}

void MySQLConnection::ExecuteInsert(const std::string& insert_statement,
                                    io::ParamsBinderBase& params,
                                    engine::Deadline deadline) {
  ExecuteStatement(insert_statement, params, deadline, std::nullopt);
}

void MySQLConnection::Ping(engine::Deadline deadline) {
  auto guard = GetBrokenGuard();

  guard.Execute([this, deadline] {
    int err = MySQLNativeInterface{socket_, deadline}.Ping(&mysql_);

    if (err != 0) {
      throw MySQLException{mysql_errno(&mysql_),
                           GetNativeError("Failed to ping the server")};
    }
  });
}

void MySQLConnection::Commit(engine::Deadline deadline) {
  auto guard = GetBrokenGuard();

  guard.Execute([this, deadline] {
    my_bool err = MySQLNativeInterface{socket_, deadline}.Commit(&mysql_);

    if (err != 0) {
      throw MySQLTransactionException{
          mysql_errno(&mysql_),
          GetNativeError("Failed to commit a transaction")};
    }
  });
}

void MySQLConnection::Rollback(engine::Deadline deadline) {
  auto guard = GetBrokenGuard();

  guard.Execute([this, deadline] {
    my_bool err = MySQLNativeInterface{socket_, deadline}.Rollback(&mysql_);

    if (err != 0) {
      throw MySQLTransactionException{
          mysql_errno(&mysql_),
          GetNativeError("Failed to rollback a transaction")};
    }
  });
}

MySQLSocket& MySQLConnection::GetSocket() { return socket_; }

MYSQL& MySQLConnection::GetNativeHandler() { return mysql_; }

bool MySQLConnection::IsBroken() const { return broken_.load(); }

const char* MySQLConnection::GetNativeError() { return mysql_error(&mysql_); }

std::string MySQLConnection::GetNativeError(std::string_view prefix) {
  return fmt::format("{}: error({}) [{}] \"{}\"", prefix, mysql_errno(&mysql_),
                     mysql_sqlstate(&mysql_), mysql_error(&mysql_));
}

std::string MySQLConnection::EscapeString(std::string_view source) {
  auto buffer = std::make_unique<char[]>(source.length() * 2 + 1);
  std::size_t escaped_length = mysql_real_escape_string(
      &mysql_, buffer.get(), source.data(), source.length());

  return std::string(buffer.get(), escaped_length);
}

const metadata::MySQLServerInfo& MySQLConnection::GetServerInfo() const {
  return server_info_;
}

BrokenGuard MySQLConnection::GetBrokenGuard() { return BrokenGuard{*this}; }

void MySQLConnection::NotifyBroken() { broken_.store(true); }

void MySQLConnection::InitSocket(
    clients::dns::Resolver& resolver,
    const settings::EndpointInfo& endpoint_info,
    const settings::AuthSettings& auth_settings,
    const settings::ConnectionSettings& connection_settings,
    engine::Deadline deadline) {
  const auto addr_vec = resolver.Resolve(endpoint_info.host, deadline);

  for (const auto& addr : addr_vec) {
    const auto ip_mode = connection_settings.ip_mode;
    const auto addr_domain = addr.Domain();
    if (ip_mode != settings::IpMode::kAny) {
      if ((addr_domain == engine::io::AddrDomain::kInet6 &&
           ip_mode != settings::IpMode::kIpV6) ||
          (addr_domain == engine::io::AddrDomain::kInet &&
           ip_mode != settings::IpMode::kIpV4)) {
        continue;
      }
    }

    const auto ip = addr.PrimaryAddressString();
    if (DoInitSocket(ip, endpoint_info.port, auth_settings, connection_settings,
                     deadline)) {
      return;
    } else if (deadline.IsReached()) {
      break;
    }
  }

  throw MySQLException{
      0, "Failed to connect to any of the the resolved addresses"};
}

bool MySQLConnection::DoInitSocket(
    const std::string& ip, std::uint32_t port,
    const settings::AuthSettings& auth_settings,
    const settings::ConnectionSettings& connection_settings,
    engine::Deadline deadline) {
  mysql_init(&mysql_);
  SetNativeOptions(&mysql_, connection_settings);

  MYSQL* connect_res{nullptr};

  auto mysql_events = mysql_real_connect_start(
      &connect_res, &mysql_, ip.c_str(), auth_settings.user.c_str(),
      auth_settings.password.c_str(), auth_settings.database.c_str(), port,
      nullptr /* unix_socket */, 0 /* client_flags */);

  const auto fd = mysql_get_socket(&mysql_);
  if (fd == -1) {
    LOG_WARNING() << GetNativeError("Failed to connect");
    // Socket is invalid, so we can just call blocking version
    mysql_close(&mysql_);
    return false;
  }

  socket_.SetFd(fd);
  try {
    socket_.RunToCompletion([mysql_events] { return mysql_events; },
                            [this, &connect_res](int mysql_events) {
                              return mysql_real_connect_cont(
                                  &connect_res, &mysql_, mysql_events);
                            },
                            deadline);
  } catch (const std::exception& ex) {
    // TODO : is it safe to call close here?
    Close(deadline);
    return false;
  }

  if (!connect_res) {
    // If we call mysql_close here - we SEGFAULT.
    // If we don't - we leak.
    // https://jira.mariadb.org/browse/CONC-622
    // TODO : this must be fixed somehow
    mysql_close(&mysql_);
    LOG_WARNING() << GetNativeError("Failed to connect");
    return false;
  }

  return true;
}

void MySQLConnection::Close(engine::Deadline deadline) noexcept {
  UASSERT(socket_.IsValid());

  MySQLNativeInterface{socket_, deadline}.Close(&mysql_);
}

MySQLStatement& MySQLConnection::PrepareStatement(
    const std::string& statement, engine::Deadline deadline,
    std::optional<std::size_t> batch_size) {
  auto& mysql_statement =
      statements_cache_.PrepareStatement(statement, deadline);

  if (batch_size.has_value()) {
    UASSERT(*batch_size > 0);
    mysql_statement.SetReadonlyCursor(*batch_size);
  } else {
    mysql_statement.SetNoCursor();
  }

  return mysql_statement;
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
