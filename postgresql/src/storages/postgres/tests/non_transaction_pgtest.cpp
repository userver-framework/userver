#include <storages/postgres/tests/util_pgtest.hpp>

#include <storages/postgres/detail/non_transaction.hpp>
#include <storages/postgres/exceptions.hpp>

namespace pg = storages::postgres;

POSTGRE_TEST_P(NonTransactionSelectOne) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  pg::detail::NonTransaction ntrx(std::move(conn));

  pg::ResultSet res{nullptr};
  EXPECT_NO_THROW(res = ntrx.Execute("SELECT 1"));
  EXPECT_EQ(1, res.AsSingleRow<int>());
}

POSTGRE_TEST_P(NonTransactionExecuteTimeout) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  pg::detail::NonTransaction ntrx(std::move(conn));

  pg::CommandControl cc{std::chrono::milliseconds{50},
                        std::chrono::milliseconds{300}};
  EXPECT_THROW(ntrx.Execute(cc, "SELECT pg_sleep(1)"),
               pg::ConnectionTimeoutError);
  // NOTE: connection is now in bad state and will be discarded
  EXPECT_ANY_THROW(ntrx.Execute("SELECT 1"));
}

POSTGRE_TEST_P(NonTransactionStatementTimeout) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";
  pg::detail::NonTransaction ntrx(std::move(conn));

  pg::CommandControl cc{std::chrono::milliseconds{300},
                        std::chrono::milliseconds{50}};
  EXPECT_THROW(ntrx.Execute(cc, "SELECT pg_sleep(1)"), pg::QueryCanceled);
  // NOTE: connection may now be reused
  EXPECT_NO_THROW(ntrx.Execute("SELECT 1"));
}
