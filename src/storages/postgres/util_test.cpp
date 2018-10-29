#include <storages/postgres/util_test.hpp>

#include <postgresql/libpq-fe.h>

#include <boost/algorithm/string.hpp>

#include <engine/standalone.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/exceptions.hpp>

namespace pg = storages::postgres;

std::vector<pg::DSNList> GetDsnFromEnv() {
  auto* conn_list_env = std::getenv(kPostgresDsn);
  if (!conn_list_env) {
    return {};
  }

  std::vector<std::string> conn_list;
  boost::split(conn_list, conn_list_env, [](char c) { return c == ';'; },
               boost::token_compress_on);

  std::vector<pg::DSNList> dsns_list;
  std::transform(conn_list.begin(), conn_list.end(),
                 std::back_inserter(dsns_list), pg::SplitByHost);
  return dsns_list;
}

std::string DsnToString(const ::testing::TestParamInfo<pg::DSNList>& info) {
  if (info.param.empty()) {
    return {};
  }
  return pg::MakeDsnNick(info.param[0]);
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
  static auto task_processor_holder =
      engine::impl::TaskProcessorHolder::MakeTaskProcessor(
          1, "close_pg_connection", engine::impl::MakeTaskProcessorPools());
  return *task_processor_holder;
}
