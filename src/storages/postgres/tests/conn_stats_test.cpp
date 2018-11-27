#include <storages/postgres/tests/util_test.hpp>

#include <gtest/gtest.h>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/pool.hpp>

namespace pg = storages::postgres;

namespace {

class PostgreStats : public PostgreSQLBase,
                     public ::testing::WithParamInterface<std::string> {
  void ReadParam() override { dsn_ = GetParam(); }

 protected:
  std::string dsn_;
};

INSTANTIATE_TEST_CASE_P(/*empty*/, PostgreStats,
                        ::testing::ValuesIn(GetDsnFromEnv()), DsnToString);

TEST_P(PostgreStats, NoTransactions) {
  RunInCoro([this] {
    pg::detail::ConnectionPtr conn;
    EXPECT_NO_THROW(
        conn = pg::detail::Connection::Connect(dsn_, GetTaskProcessor()))
        << "Connect to correct DSN";
    ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

    // We can't check all the counters as some of them are used for internal ops
    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(stats.trx_total, 0);
    EXPECT_EQ(stats.commit_total, 0);
    EXPECT_EQ(stats.rollback_total, 0);
    EXPECT_EQ(stats.error_execute_total, 0);
    EXPECT_EQ(stats.trx_start_time.time_since_epoch().count(), 0);
    EXPECT_EQ(stats.trx_end_time.time_since_epoch().count(), 0);
  });
}

TEST_P(PostgreStats, StatsResetAfterGet) {
  RunInCoro([this] {
    pg::detail::ConnectionPtr conn;
    EXPECT_NO_THROW(
        conn = pg::detail::Connection::Connect(dsn_, GetTaskProcessor()))
        << "Connect to correct DSN";
    ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

    std::ignore = conn->GetStatsAndReset();
    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(stats.trx_total, 0);
    EXPECT_EQ(stats.commit_total, 0);
    EXPECT_EQ(stats.rollback_total, 0);
    EXPECT_EQ(stats.parse_total, 0);
    EXPECT_EQ(stats.execute_total, 0);
    EXPECT_EQ(stats.reply_total, 0);
    EXPECT_EQ(stats.bin_reply_total, 0);
    EXPECT_EQ(stats.error_execute_total, 0);
    EXPECT_EQ(stats.trx_start_time.time_since_epoch().count(), 0);
    EXPECT_EQ(stats.trx_end_time.time_since_epoch().count(), 0);
    EXPECT_EQ(stats.sum_query_duration.count(), 0);
  });
}

TEST_P(PostgreStats, TransactionStartTime) {
  RunInCoro([this] {
    pg::detail::ConnectionPtr conn;
    EXPECT_NO_THROW(
        conn = pg::detail::Connection::Connect(dsn_, GetTaskProcessor()))
        << "Connect to correct DSN";
    ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

    std::ignore = conn->GetStatsAndReset();
    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(stats.trx_total, 0);
    EXPECT_EQ(stats.commit_total, 0);
    EXPECT_EQ(stats.rollback_total, 0);
    EXPECT_EQ(stats.parse_total, 0);
    EXPECT_EQ(stats.execute_total, 0);
    EXPECT_EQ(stats.reply_total, 0);
    EXPECT_EQ(stats.bin_reply_total, 0);
    EXPECT_EQ(stats.error_execute_total, 0);
    EXPECT_EQ(stats.trx_start_time.time_since_epoch().count(), 0);
    EXPECT_EQ(stats.trx_end_time.time_since_epoch().count(), 0);
    EXPECT_EQ(stats.sum_query_duration.count(), 0);
  });
}

TEST_P(PostgreStats, TransactionExecuted) {
  RunInCoro([this] {
    pg::detail::ConnectionPtr conn;
    EXPECT_NO_THROW(
        conn = pg::detail::Connection::Connect(dsn_, GetTaskProcessor()))
        << "Connect to correct DSN";
    ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

    std::ignore = conn->GetStatsAndReset();
    const auto time_start = pg::detail::SteadyClock::now();
    EXPECT_NO_THROW(
        conn->Begin(pg::TransactionOptions{}, pg::detail::SteadyClock::now()));
    EXPECT_NO_THROW(conn->Execute("select 1"))
        << "select 1 successfully executed";
    EXPECT_NO_THROW(conn->Commit());

    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(stats.trx_total, 1);
    EXPECT_EQ(stats.commit_total, 1);
    EXPECT_EQ(stats.rollback_total, 0);
    EXPECT_EQ(stats.parse_total, 1);
    EXPECT_EQ(stats.execute_total, 3);
    EXPECT_EQ(stats.reply_total, 1);
    EXPECT_EQ(stats.bin_reply_total, 1);
    EXPECT_EQ(stats.error_execute_total, 0);
    EXPECT_GT(stats.trx_start_time, time_start);
    EXPECT_GT(stats.trx_end_time, time_start);
    EXPECT_GT(stats.sum_query_duration.count(), 0);
  });
}

TEST_P(PostgreStats, TransactionFailed) {
  RunInCoro([this] {
    pg::detail::ConnectionPtr conn;
    EXPECT_NO_THROW(
        conn = pg::detail::Connection::Connect(dsn_, GetTaskProcessor()))
        << "Connect to correct DSN";
    ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

    std::ignore = conn->GetStatsAndReset();
    const auto time_start = pg::detail::SteadyClock::now();
    EXPECT_NO_THROW(
        conn->Begin(pg::TransactionOptions{}, pg::detail::SteadyClock::now()));
    EXPECT_THROW(conn->Execute("select select select"), pg::Error)
        << "invalid query throws exception";
    EXPECT_NO_THROW(conn->Rollback());

    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(stats.trx_total, 1);
    EXPECT_EQ(stats.commit_total, 0);
    EXPECT_EQ(stats.rollback_total, 1);
    EXPECT_EQ(stats.parse_total, 0);
    EXPECT_EQ(stats.execute_total, 3);
    EXPECT_EQ(stats.reply_total, 0);
    EXPECT_EQ(stats.bin_reply_total, 0);
    EXPECT_EQ(stats.error_execute_total, 1);
    EXPECT_GT(stats.trx_start_time, time_start);
    EXPECT_GT(stats.trx_end_time, time_start);
    EXPECT_GT(stats.sum_query_duration.count(), 0);
  });
}

TEST_P(PostgreStats, TransactionMultiExecutions) {
  RunInCoro([this] {
    pg::detail::ConnectionPtr conn;
    EXPECT_NO_THROW(
        conn = pg::detail::Connection::Connect(dsn_, GetTaskProcessor()))
        << "Connect to correct DSN";
    ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

    const auto exec_count = 10;
    std::ignore = conn->GetStatsAndReset();
    EXPECT_NO_THROW(
        conn->Begin(pg::TransactionOptions{}, pg::detail::SteadyClock::now()));
    for (auto i = 0; i < exec_count; ++i) {
      EXPECT_NO_THROW(conn->Execute("select 1"))
          << "select 1 successfully executed";
    }
    EXPECT_NO_THROW(conn->Commit());

    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(stats.trx_total, 1);
    EXPECT_EQ(stats.commit_total, 1);
    EXPECT_EQ(stats.rollback_total, 0);
    EXPECT_EQ(stats.parse_total, 1);
    EXPECT_EQ(stats.execute_total, exec_count + 2);
    EXPECT_EQ(stats.reply_total, exec_count);
    EXPECT_EQ(stats.bin_reply_total, exec_count);
    EXPECT_EQ(stats.error_execute_total, 0);
  });
}

}  // namespace
