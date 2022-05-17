#include <storages/postgres/tests/util_pgtest.hpp>

#include <boost/algorithm/string.hpp>

#include <userver/engine/task/task.hpp>

#include <storages/postgres/default_command_controls.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

namespace {
constexpr const char* kPostgresDsn = "POSTGRES_TEST_DSN";
constexpr const char* kPostgresLog = "POSTGRES_TEST_LOG";
}  // namespace

pg::DefaultCommandControls GetTestCmdCtls() {
  static auto kDefaultCmdCtls = pg::DefaultCommandControls(kTestCmdCtl, {}, {});
  return kDefaultCmdCtls;
}

DefaultCommandControlScope::DefaultCommandControlScope(
    storages::postgres::CommandControl default_cmd_ctl)
    : old_cmd_ctl_(GetTestCmdCtls().GetDefaultCmdCtl()) {
  GetTestCmdCtls().UpdateDefaultCmdCtl(default_cmd_ctl);
}

DefaultCommandControlScope::~DefaultCommandControlScope() {
  GetTestCmdCtls().UpdateDefaultCmdCtl(old_cmd_ctl_);
}

engine::Deadline MakeDeadline() {
  return engine::Deadline::FromDuration(kTestCmdCtl.execute);
}

void PrintBuffer(std::ostream& os, const std::uint8_t* buffer,
                 std::size_t size) {
  os << "Buffer size " << size << '\n';
  std::size_t b_no{0};
  std::ostringstream printable;
  for (auto c = buffer; c != buffer + size; ++c) {
    unsigned char byte = *c;
    os << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    printable << (std::isprint(*c) ? *c : '.');
    ++b_no;
    if (b_no % 16 == 0) {
      os << '\t' << printable.str() << '\n';
      printable.str(std::string{});
    } else if (b_no % 8 == 0) {
      os << "   ";
      printable << " ";
    } else {
      os << " ";
    }
  }
  auto remain = 16 - b_no % 16;
  os << std::dec << std::setw(remain * 3 - 1) << std::setfill(' ') << ' '
     << '\t' << printable.str() << '\n';
}

void PrintBuffer(std::ostream& os, const std::string& buffer) {
  PrintBuffer(os, reinterpret_cast<const std::uint8_t*>(buffer.data()),
              buffer.size());
}

PostgreSQLBase::PostgreSQLBase() {
  if (std::getenv(kPostgresLog)) {
    old_ = logging::SetDefaultLogger(logging::MakeStderrLogger(
        "cerr", logging::Format::kTskv, logging::Level::kDebug));
  }
}

PostgreSQLBase::~PostgreSQLBase() {
  if (old_) {
    logging::SetDefaultLogger(std::move(old_));
  }
}

pg::Dsn PostgreSQLBase::GetDsnFromEnv() {
  auto dsn_list = GetDsnListFromEnv();
  return dsn_list.empty() ? pg::Dsn{"postgresql://"} : std::move(dsn_list[0]);
}

pg::DsnList PostgreSQLBase::GetDsnListFromEnv() {
  auto* conn_list_env = std::getenv(kPostgresDsn);
  if (!conn_list_env) {
    return {};
  }

  std::vector<std::string> conn_list;
  boost::split(
      conn_list, conn_list_env, [](char c) { return c == ';'; },
      boost::token_compress_on);

  pg::DsnList dsn_list;
  for (auto conn : conn_list) {
    dsn_list.insert(dsn_list.end(), pg::Dsn{std::move(conn)});
  }
  return dsn_list;
}

storages::postgres::detail::ConnectionPtr PostgreSQLBase::MakeConnection(
    const storages::postgres::Dsn& dsn, engine::TaskProcessor& task_processor,
    storages::postgres::ConnectionSettings settings) {
  std::unique_ptr<pg::detail::Connection> conn;

  UEXPECT_NO_THROW(conn = pg::detail::Connection::Connect(
                       dsn, nullptr, task_processor, kConnectionId, settings,
                       GetTestCmdCtls(), {}, {}))
      << "Connect to correct DSN";
  pg::detail::ConnectionPtr conn_ptr{std::move(conn)};
  CheckConnection(conn_ptr);
  return conn_ptr;
}

void PostgreSQLBase::CheckConnection(const pg::detail::ConnectionPtr& conn) {
  ASSERT_TRUE(conn) << "Expected non-empty connection pointer";

  EXPECT_TRUE(conn->IsConnected()) << "Connection to PostgreSQL is established";
  EXPECT_TRUE(conn->IsIdle())
      << "Connection to PosgreSQL is idle after connection";
  EXPECT_FALSE(conn->IsInTransaction()) << "Connection to PostgreSQL is "
                                           "not in a transaction after "
                                           "connection";
}

void PostgreSQLBase::FinalizeConnection(pg::detail::ConnectionPtr conn) {
  UEXPECT_NO_THROW(conn->Close()) << "Successfully close connection";
  EXPECT_FALSE(conn->IsConnected()) << "Connection is not connected";
  EXPECT_FALSE(conn->IsIdle()) << "Connection is not idle";
  EXPECT_FALSE(conn->IsInTransaction())
      << "Connection to PostgreSQL is not in a transaction after closing";
}

engine::TaskProcessor& PostgreSQLBase::GetTaskProcessor() {
  return engine::current_task::GetTaskProcessor();
}

PostgreConnection::PostgreConnection()
    : conn(MakeConnection(GetDsnFromEnv(), GetTaskProcessor(), GetParam())) {}

PostgreConnection::~PostgreConnection() {
  // force connection cleanup to avoid leaving detached tasks behind
  engine::AsyncNoSpan(GetTaskProcessor(), [] {}).Wait();
}

INSTANTIATE_UTEST_SUITE_P(ConnectionSettings, PostgreConnection,
                          ::testing::Values(kCachePreparedStatements,
                                            kPipelineEnabled));

USERVER_NAMESPACE_END
