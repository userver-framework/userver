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
    auto conn = MakeConnection(dsn_, GetTaskProcessor());

    // We can't check all the counters as some of them are used for internal ops
    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(0, stats.trx_total);
    EXPECT_EQ(0, stats.commit_total);
    EXPECT_EQ(0, stats.rollback_total);
    EXPECT_EQ(0, stats.out_of_trx);
    EXPECT_EQ(0, stats.error_execute_total);
    EXPECT_EQ(0, stats.trx_start_time.time_since_epoch().count());
    EXPECT_EQ(0, stats.trx_end_time.time_since_epoch().count());
  });
}

TEST_P(PostgreStats, StatsResetAfterGet) {
  RunInCoro([this] {
    auto conn = MakeConnection(dsn_, GetTaskProcessor());

    std::ignore = conn->GetStatsAndReset();
    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(0, stats.trx_total);
    EXPECT_EQ(0, stats.commit_total);
    EXPECT_EQ(0, stats.rollback_total);
    EXPECT_EQ(0, stats.out_of_trx);
    EXPECT_EQ(0, stats.parse_total);
    EXPECT_EQ(0, stats.execute_total);
    EXPECT_EQ(0, stats.reply_total);
    EXPECT_EQ(0, stats.bin_reply_total);
    EXPECT_EQ(0, stats.error_execute_total);
    EXPECT_EQ(0, stats.trx_start_time.time_since_epoch().count());
    EXPECT_EQ(0, stats.trx_end_time.time_since_epoch().count());
    EXPECT_EQ(0, stats.sum_query_duration.count());
  });
}

TEST_P(PostgreStats, TransactionStartTime) {
  RunInCoro([this] {
    auto conn = MakeConnection(dsn_, GetTaskProcessor());

    std::ignore = conn->GetStatsAndReset();
    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(0, stats.trx_total);
    EXPECT_EQ(0, stats.commit_total);
    EXPECT_EQ(0, stats.rollback_total);
    EXPECT_EQ(0, stats.parse_total);
    EXPECT_EQ(0, stats.out_of_trx);
    EXPECT_EQ(0, stats.execute_total);
    EXPECT_EQ(0, stats.reply_total);
    EXPECT_EQ(0, stats.bin_reply_total);
    EXPECT_EQ(0, stats.error_execute_total);
    EXPECT_EQ(0, stats.trx_start_time.time_since_epoch().count());
    EXPECT_EQ(0, stats.trx_end_time.time_since_epoch().count());
    EXPECT_EQ(0, stats.sum_query_duration.count());
  });
}

TEST_P(PostgreStats, TransactionExecuted) {
  RunInCoro([this] {
    auto conn = MakeConnection(dsn_, GetTaskProcessor());

    std::ignore = conn->GetStatsAndReset();
    const auto time_start = pg::detail::SteadyClock::now();
    EXPECT_NO_THROW(
        conn->Begin(pg::TransactionOptions{}, pg::detail::SteadyClock::now()));
    EXPECT_NO_THROW(conn->Execute("select 1"))
        << "select 1 successfully executed";
    EXPECT_NO_THROW(conn->Commit());

    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(1, stats.trx_total);
    EXPECT_EQ(1, stats.commit_total);
    EXPECT_EQ(0, stats.rollback_total);
    EXPECT_EQ(0, stats.out_of_trx);
    EXPECT_EQ(1, stats.parse_total);
    EXPECT_EQ(3, stats.execute_total);
    EXPECT_EQ(1, stats.reply_total);
    EXPECT_EQ(1, stats.bin_reply_total);
    EXPECT_EQ(0, stats.error_execute_total);
    EXPECT_GT(stats.trx_start_time, time_start);
    EXPECT_GT(stats.trx_end_time, time_start);
    EXPECT_GT(stats.sum_query_duration.count(), 0);
  });
}

TEST_P(PostgreStats, TransactionFailed) {
  RunInCoro([this] {
    auto conn = MakeConnection(dsn_, GetTaskProcessor());

    std::ignore = conn->GetStatsAndReset();
    const auto time_start = pg::detail::SteadyClock::now();
    EXPECT_NO_THROW(
        conn->Begin(pg::TransactionOptions{}, pg::detail::SteadyClock::now()));
    EXPECT_THROW(conn->Execute("select select select"), pg::Error)
        << "invalid query throws exception";
    EXPECT_NO_THROW(conn->Rollback());

    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(1, stats.trx_total);
    EXPECT_EQ(0, stats.commit_total);
    EXPECT_EQ(1, stats.rollback_total);
    EXPECT_EQ(0, stats.out_of_trx);
    EXPECT_EQ(0, stats.parse_total);
    EXPECT_EQ(3, stats.execute_total);
    EXPECT_EQ(0, stats.reply_total);
    EXPECT_EQ(0, stats.bin_reply_total);
    EXPECT_EQ(1, stats.error_execute_total);
    EXPECT_GT(stats.trx_start_time, time_start);
    EXPECT_GT(stats.trx_end_time, time_start);
    EXPECT_GT(stats.sum_query_duration.count(), 0);
  });
}

TEST_P(PostgreStats, TransactionMultiExecutions) {
  RunInCoro([this] {
    auto conn = MakeConnection(dsn_, GetTaskProcessor());

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
    EXPECT_EQ(1, stats.trx_total);
    EXPECT_EQ(1, stats.commit_total);
    EXPECT_EQ(0, stats.rollback_total);
    EXPECT_EQ(0, stats.out_of_trx);
    EXPECT_EQ(1, stats.parse_total);
    EXPECT_EQ(exec_count + 2, stats.execute_total);
    EXPECT_EQ(exec_count, stats.reply_total);
    EXPECT_EQ(exec_count, stats.bin_reply_total);
    EXPECT_EQ(0, stats.error_execute_total);
  });
}

TEST_P(PostgreStats, SingleQuery) {
  RunInCoro([this] {
    auto conn = MakeConnection(dsn_, GetTaskProcessor());

    std::ignore = conn->GetStatsAndReset();

    EXPECT_NO_THROW(conn->Start(pg::detail::SteadyClock::now()));
    EXPECT_NO_THROW(conn->Execute("select 1"));
    EXPECT_NO_THROW(conn->Finish());

    const auto stats = conn->GetStatsAndReset();
    EXPECT_EQ(1, stats.trx_total);
    EXPECT_EQ(0, stats.commit_total);
    EXPECT_EQ(0, stats.rollback_total);
    EXPECT_EQ(1, stats.out_of_trx);
    EXPECT_EQ(1, stats.parse_total);
    EXPECT_EQ(1, stats.execute_total);
    EXPECT_EQ(1, stats.reply_total);
    EXPECT_EQ(1, stats.bin_reply_total);
    EXPECT_EQ(0, stats.error_execute_total);
  });
}

}  // namespace
