#include <storages/mysql/impl/connection.hpp>

// for std::cerr in AbortOnBuggyLibmariadb
#include <iostream>

#include <fmt/format.h>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/scope_time.hpp>

#include <storages/mysql/impl/metadata/native_client_info.hpp>
#include <storages/mysql/impl/metadata/server_info.hpp>
#include <storages/mysql/impl/native_interface.hpp>
#include <storages/mysql/impl/plain_query.hpp>
#include <storages/mysql/settings/settings.hpp>
#include <userver/storages/mysql/exceptions.hpp>
#include <userver/storages/mysql/impl/io/extractor.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace {

class MysqlThreadInitRegistrator final {
 public:
  MysqlThreadInitRegistrator() {
    engine::RegisterThreadStartedHook([] { mysql_thread_init(); });
  }
};

[[maybe_unused]] const MysqlThreadInitRegistrator init_registrator{};

class MysqlThreadEnd final {
 public:
  ~MysqlThreadEnd() { mysql_thread_end(); }
};

// quoting the docs:
// "mysql_close() sends a COM_QUIT request to the server, though it does not
// wait for any reply. Thus, theoretically it can block (if the socket buffer
// is full), though in practise it is probably unlikely to occur frequently."
//
// If it blocks we use `shutdown` on the socket, because there is no other way
// to honor any kind of deadline, so this is zero.
constexpr std::chrono::milliseconds kDefaultCloseTimeout{0};

#ifdef USERVER_MYSQL_ALLOW_BUGGY_LIBMARIADB
constexpr bool kAbortOnBuggyLibmariadb = false;
#else
constexpr bool kAbortOnBuggyLibmariadb = true;
#endif

[[maybe_unused]] [[noreturn]] void AbortOnBuggyLibmariadb(
    metadata::SemVer client_version_id) {
  constexpr std::string_view kBuggyLibmariadbTemplate{
      "Your version of libmariadb3 ({}) has a critical bug, which in some "
      "cases of connection setup failure leads to either UB or memory leaks: "
      "https://jira.mariadb.org/projects/CONC/issues/CONC-622.\n"
      "Please either update libmariadb3 to at least 3.3.4, or set "
      "USERVER_MYSQL_ALLOW_BUGGY_LIBMARIADB cmake variable to disable this "
      "assert and leak instead in aforementioned situations.\n"};

  const auto message =
      fmt::format(kBuggyLibmariadbTemplate, client_version_id.ToString());

  LOG_CRITICAL() << message;
  logging::LogFlush();

  std::cerr << message << std::endl;

  std::abort();
}

void SetNativeOptions(MYSQL* mysql,
                      const settings::ConnectionSettings& connection_settings) {
  UASSERT(mysql);
  // making everything async
  mysql_optionsv(mysql, MYSQL_OPT_NONBLOCK, nullptr);
  // no automatic reconnect
  my_bool reconnect = 0;
  mysql_optionsv(mysql, MYSQL_OPT_RECONNECT, &reconnect);
  // report MYSQL_DATA_TRUNCATED for prepared statements fetch
  mysql_optionsv(mysql, MYSQL_REPORT_DATA_TRUNCATION, "1");
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

Connection::Connection(clients::dns::Resolver& resolver,
                       const settings::EndpointInfo& endpoint_info,
                       const settings::AuthSettings& auth_settings,
                       const settings::ConnectionSettings& connection_settings,
                       engine::Deadline deadline)
    : socket_{-1, 0},
      statements_cache_{*this, connection_settings.statements_cache_size} {
  static thread_local MysqlThreadEnd mysql_init{};

  InitSocket(resolver, endpoint_info, auth_settings, connection_settings,
             deadline);
  server_info_ = metadata::ServerInfo::Get(mysql_);

  const auto server_info = metadata::ServerInfo::Get(mysql_);
  LOG_INFO() << "MySQL connection initialized."
             << " Server type: " << server_info.server_type_str << " "
             << server_info.server_version.ToString();
}

Connection::~Connection() {
  // We close the connection before resetting statements cache, so that reset
  // doesn't do potentially costly I/O
  Close(engine::Deadline::FromDuration(kDefaultCloseTimeout));
}

QueryResult Connection::ExecuteQuery(const std::string& query,
                                     engine::Deadline deadline) {
  auto guard = GetBrokenGuard();

  return guard.Execute([&] {
    PlainQuery mysql_query{*this, query};
    mysql_query.Execute(deadline);
    return mysql_query.FetchResult(deadline);
  });
}

StatementFetcher Connection::ExecuteStatement(
    const std::string& statement, io::ParamsBinderBase& params,
    engine::Deadline deadline, std::optional<std::size_t> batch_size) {
  auto guard = GetBrokenGuard();

  return guard.Execute([&] {
    auto& mysql_statement = PrepareStatement(statement, deadline, batch_size);

    return mysql_statement.Execute(params, deadline);
  });
}

void Connection::Ping(engine::Deadline deadline) {
  auto guard = GetBrokenGuard();

  guard.Execute([this, deadline] {
    const int err = NativeInterface{socket_, deadline}.Ping(&mysql_);

    if (err != 0) {
      throw MySQLException{mysql_errno(&mysql_),
                           GetNativeError("Failed to ping the server")};
    }
  });
}

void Connection::Commit(engine::Deadline deadline) {
  auto guard = GetBrokenGuard();

  guard.Execute([this, deadline] {
    const my_bool err = NativeInterface{socket_, deadline}.Commit(&mysql_);

    if (err != 0) {
      throw MySQLTransactionException{
          mysql_errno(&mysql_),
          GetNativeError("Failed to commit a transaction")};
    }
  });
}

void Connection::Rollback(engine::Deadline deadline) {
  auto guard = GetBrokenGuard();

  guard.Execute([this, deadline] {
    const my_bool err = NativeInterface{socket_, deadline}.Rollback(&mysql_);

    if (err != 0) {
      throw MySQLTransactionException{
          mysql_errno(&mysql_),
          GetNativeError("Failed to rollback a transaction")};
    }
  });
}

Socket& Connection::GetSocket() { return socket_; }

MYSQL& Connection::GetNativeHandler() { return mysql_; }

bool Connection::IsBroken() const { return broken_.load(); }

std::string Connection::GetNativeError(std::string_view prefix) {
  return fmt::format("{}: error({}) [{}] \"{}\"", prefix, mysql_errno(&mysql_),
                     mysql_sqlstate(&mysql_), mysql_error(&mysql_));
}

const metadata::ServerInfo& Connection::GetServerInfo() const {
  return server_info_;
}

BrokenGuard Connection::GetBrokenGuard() { return BrokenGuard{*this}; }

void Connection::NotifyBroken() { broken_.store(true); }

void Connection::InitSocket(
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

bool Connection::DoInitSocket(
    const std::string& ip, std::uint32_t port,
    const settings::AuthSettings& auth_settings,
    const settings::ConnectionSettings& connection_settings,
    engine::Deadline deadline) {
  mysql_init(&mysql_);
  SetNativeOptions(&mysql_, connection_settings);

  MYSQL* connect_res{nullptr};

  auto mysql_events = mysql_real_connect_start(
      &connect_res, &mysql_, ip.c_str(), auth_settings.user.c_str(),
      auth_settings.password.GetUnderlying().c_str(),
      auth_settings.database.c_str(), port, nullptr /* unix_socket */,
      0 /* client_flags */);

  const auto fd = mysql_get_socket(&mysql_);
  if (fd == -1) {
    LOG_WARNING() << GetNativeError("Failed to connect");
    // Socket is invalid, so we can just call blocking version - since there's
    // no socket no I/O can possibly be done, thus there's no way to block.
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
    const auto client_version_id =
        metadata::NativeClientInfo::Get().client_version_id;
    if (client_version_id < metadata::SemVer{3, 3, 4}) {
      if constexpr (kAbortOnBuggyLibmariadb) {
        AbortOnBuggyLibmariadb(client_version_id);
      } else {
        // This is absolutely critical, because in case of host maintenance
        // (or any other unavailability) we will create and leak the connection
        // every time pool is performing its maintenance.
        // The decision was made to allow executing with <3.3.4 because 3.3.4 is
        // very recent at the time of writing, and it's probably too restricting
        // to refuse to start with lower versions.
        LOG_CRITICAL()
            << "Can't clean up connection resources due to "
               "https://jira.mariadb.org/browse/CONC-622, connection will "
               "be leaked. Consider upgrading libmariadb3 to 3.3.4 or higher.";
      }
    } else {
      mysql_close(&mysql_);
    }
    LOG_WARNING() << GetNativeError("Failed to connect");
    return false;
  }

  return true;
}

void Connection::Close(engine::Deadline deadline) noexcept {
  UASSERT(socket_.IsValid());

  NativeInterface{socket_, deadline}.Close(&mysql_);
}

Statement& Connection::PrepareStatement(const std::string& statement,
                                        engine::Deadline deadline,
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
