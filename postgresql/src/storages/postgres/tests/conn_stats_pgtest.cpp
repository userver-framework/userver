#include <storages/postgres/tests/util_pgtest.hpp>

#include <gtest/gtest.h>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/pool.hpp>

namespace pg = storages::postgres;

namespace {

class PostgreStats : public PostgreConnection {};

INSTANTIATE_POSTGRE_CASE_P(PostgreStats);

POSTGRE_CASE_TEST_P(PostgreStats, NoTransactions) {
  // We can't check all the counters as some of them are used for internal ops
  const auto stats = conn->GetStatsAndReset();
  EXPECT_EQ(0, stats.trx_total);
  EXPECT_EQ(0, stats.commit_total);
  EXPECT_EQ(0, stats.rollback_total);
  EXPECT_EQ(0, stats.out_of_trx);
  EXPECT_EQ(0, stats.error_execute_total);
  EXPECT_EQ(0, stats.trx_start_time.time_since_epoch().count());
  EXPECT_EQ(0, stats.trx_end_time.time_since_epoch().count());
}

POSTGRE_CASE_TEST_P(PostgreStats, StatsResetAfterGet) {
  std::ignore = conn->GetStatsAndReset();
  const auto stats = conn->GetStatsAndReset();
  EXPECT_EQ(0, stats.trx_total);
  EXPECT_EQ(0, stats.commit_total);
  EXPECT_EQ(0, stats.rollback_total);
  EXPECT_EQ(0, stats.out_of_trx);
  EXPECT_EQ(0, stats.parse_total);
  EXPECT_EQ(0, stats.execute_total);
  EXPECT_EQ(0, stats.reply_total);
  EXPECT_EQ(0, stats.error_execute_total);
  EXPECT_EQ(0, stats.trx_start_time.time_since_epoch().count());
  EXPECT_EQ(0, stats.trx_end_time.time_since_epoch().count());
  EXPECT_EQ(0, stats.sum_query_duration.count());
}

POSTGRE_CASE_TEST_P(PostgreStats, TransactionStartTime) {
  std::ignore = conn->GetStatsAndReset();
  const auto stats = conn->GetStatsAndReset();
  EXPECT_EQ(0, stats.trx_total);
  EXPECT_EQ(0, stats.commit_total);
  EXPECT_EQ(0, stats.rollback_total);
  EXPECT_EQ(0, stats.parse_total);
  EXPECT_EQ(0, stats.out_of_trx);
  EXPECT_EQ(0, stats.execute_total);
  EXPECT_EQ(0, stats.reply_total);
  EXPECT_EQ(0, stats.error_execute_total);
  EXPECT_EQ(0, stats.trx_start_time.time_since_epoch().count());
  EXPECT_EQ(0, stats.trx_end_time.time_since_epoch().count());
  EXPECT_EQ(0, stats.sum_query_duration.count());
}

POSTGRE_CASE_TEST_P(PostgreStats, TransactionExecuted) {
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
  EXPECT_EQ(0, stats.error_execute_total);
  EXPECT_GT(stats.trx_start_time, time_start);
  EXPECT_GT(stats.trx_end_time, time_start);
  EXPECT_GT(stats.sum_query_duration.count(), 0);
}

POSTGRE_CASE_TEST_P(PostgreStats, TransactionFailed) {
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
  EXPECT_EQ(1, stats.error_execute_total);
  EXPECT_GT(stats.trx_start_time, time_start);
  EXPECT_GT(stats.trx_end_time, time_start);
  EXPECT_GT(stats.sum_query_duration.count(), 0);
}

POSTGRE_CASE_TEST_P(PostgreStats, TransactionMultiExecutions) {
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
  EXPECT_EQ(0, stats.error_execute_total);
}

POSTGRE_CASE_TEST_P(PostgreStats, SingleQuery) {
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
  EXPECT_EQ(0, stats.error_execute_total);
}

}  // namespace
