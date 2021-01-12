#include <storages/postgres/tests/util_pgtest.hpp>

#include <libpq-fe.h>

#include <boost/algorithm/string.hpp>

#include <engine/standalone.hpp>

#include <storages/postgres/default_command_controls.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/exceptions.hpp>

namespace pg = storages::postgres;

pg::DefaultCommandControls GetTestCmdCtls() {
  static auto kDefaultCmdCtls = pg::DefaultCommandControls(kTestCmdCtl, {});
  return kDefaultCmdCtls;
}

std::vector<pg::Dsn> GetDsnFromEnv() {
  const auto* dsn_list_env = std::getenv(kPostgresDsn);
  return dsn_list_env ? std::vector<pg::Dsn>{pg::Dsn{dsn_list_env}}
                      : std::vector<pg::Dsn>{};
}

std::vector<pg::DsnList> GetDsnListsFromEnv() {
  auto* conn_list_env = std::getenv(kPostgresDsn);
  if (!conn_list_env) {
    return {};
  }

  std::vector<std::string> conn_list;
  boost::split(
      conn_list, conn_list_env, [](char c) { return c == ';'; },
      boost::token_compress_on);

  std::vector<pg::DsnList> dsn_lists;
  dsn_lists.reserve(conn_list.size());
  for (auto conn : conn_list) {
    dsn_lists.push_back(pg::SplitByHost(pg::Dsn{std::move(conn)}));
  }
  return dsn_lists;
}

std::string DsnToString(const ::testing::TestParamInfo<pg::Dsn>& info) {
  return pg::MakeDsnNick(info.param, true);
}

std::string DsnListToString(const ::testing::TestParamInfo<pg::DsnList>& info) {
  if (info.param.empty()) {
    return {};
  }
  return pg::MakeDsnNick(info.param[0], true);
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

void PostgreSQLBase::CheckConnection(pg::detail::ConnectionPtr conn) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

  EXPECT_TRUE(conn->IsConnected()) << "Connection to PostgreSQL is established";
  EXPECT_TRUE(conn->IsIdle())
      << "Connection to PosgreSQL is idle after connection";
  EXPECT_FALSE(conn->IsInTransaction()) << "Connection to PostgreSQL is "
                                           "not in a transaction after "
                                           "connection";
  EXPECT_EQ(pg::ConnectionState::kIdle, conn->GetState());

  EXPECT_NO_THROW(conn->Close()) << "Successfully close connection";
  EXPECT_FALSE(conn->IsConnected()) << "Connection is not connected";
  EXPECT_FALSE(conn->IsIdle()) << "Connection is not idle";
  EXPECT_FALSE(conn->IsInTransaction())
      << "Connection to PostgreSQL is not in a transaction after closing";
}

engine::TaskProcessor& PostgreSQLBase::GetTaskProcessor() {
  return engine::current_task::GetCurrentTaskContext()->GetTaskProcessor();
}

INSTANTIATE_POSTGRE_CASE_P(PostgreConnection);
