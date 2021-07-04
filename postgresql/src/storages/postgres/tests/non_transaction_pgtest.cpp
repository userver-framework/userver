#include <storages/postgres/tests/util_pgtest.hpp>

#include <userver/storages/postgres/detail/non_transaction.hpp>
#include <userver/storages/postgres/exceptions.hpp>

namespace pg = storages::postgres;

UTEST_P(PostgreConnection, NonTransactionSelectOne) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  pg::detail::NonTransaction ntrx(std::move(conn));

  pg::ResultSet res{nullptr};
  EXPECT_NO_THROW(res = ntrx.Execute("SELECT 1"));
  EXPECT_EQ(1, res.AsSingleRow<int>());
}

UTEST_P(PostgreConnection, NonTransactionExecuteTimeout) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  pg::detail::NonTransaction ntrx(std::move(conn));

  pg::CommandControl cc{std::chrono::milliseconds{50},
                        std::chrono::milliseconds{300}};
  EXPECT_THROW(ntrx.Execute(cc, "SELECT pg_sleep(1)"),
               pg::ConnectionTimeoutError);
  // NOTE: connection is now in bad state and will be discarded
  EXPECT_ANY_THROW(ntrx.Execute("SELECT 1"));
}

UTEST_P(PostgreConnection, NonTransactionStatementTimeout) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  pg::detail::NonTransaction ntrx(std::move(conn));

  pg::CommandControl cc{std::chrono::milliseconds{300},
                        std::chrono::milliseconds{50}};
  EXPECT_THROW(ntrx.Execute(cc, "SELECT pg_sleep(1)"), pg::QueryCancelled);
  // NOTE: connection may now be reused
  EXPECT_NO_THROW(ntrx.Execute("SELECT 1"));
}
