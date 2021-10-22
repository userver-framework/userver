#include <storages/postgres/tests/util_pgtest.hpp>

#include <gtest/gtest.h>

#include <storages/postgres/detail/connection.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

namespace {

class PostgreStats : public PostgreConnection {};

UTEST_F(PostgreStats, NoTransactions) {
  // We can't check all the counters as some of them are used for internal ops
  const auto stats = conn->GetStatsAndReset();
  EXPECT_EQ(0, stats.trx_total);
  EXPECT_EQ(0, stats.commit_total);
  EXPECT_EQ(0, stats.rollback_total);
  EXPECT_EQ(0, stats.out_of_trx);
  EXPECT_EQ(0, stats.portal_bind_total);
  EXPECT_EQ(0, stats.error_execute_total);
  EXPECT_EQ(0, stats.trx_start_time.time_since_epoch().count());
  EXPECT_EQ(0, stats.trx_end_time.time_since_epoch().count());
}

UTEST_F(PostgreStats, StatsResetAfterGet) {
  [[maybe_unused]] const auto old_stats = conn->GetStatsAndReset();

  const auto stats = conn->GetStatsAndReset();
  EXPECT_EQ(0, stats.trx_total);
  EXPECT_EQ(0, stats.commit_total);
  EXPECT_EQ(0, stats.rollback_total);
  EXPECT_EQ(0, stats.out_of_trx);
  EXPECT_EQ(0, stats.parse_total);
  EXPECT_EQ(0, stats.execute_total);
  EXPECT_EQ(0, stats.reply_total);
  EXPECT_EQ(0, stats.portal_bind_total);
  EXPECT_EQ(0, stats.error_execute_total);
  EXPECT_EQ(0, stats.trx_start_time.time_since_epoch().count());
  EXPECT_EQ(0, stats.trx_end_time.time_since_epoch().count());
  EXPECT_EQ(0, stats.sum_query_duration.count());
}

UTEST_F(PostgreStats, TransactionExecuted) {
  const auto old_stats = conn->GetStatsAndReset();

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
  EXPECT_EQ(0, stats.portal_bind_total);
  EXPECT_EQ(0, stats.error_execute_total);
  EXPECT_GT(stats.trx_start_time, time_start);
  EXPECT_GT(stats.trx_end_time, time_start);
  EXPECT_GT(stats.sum_query_duration.count(), 0);
  EXPECT_GT(stats.prepared_statements_current,
            old_stats.prepared_statements_current);
}

UTEST_F(PostgreStats, TransactionFailed) {
  const auto old_stats = conn->GetStatsAndReset();

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
  EXPECT_EQ(0, stats.portal_bind_total);
  EXPECT_EQ(1, stats.error_execute_total);
  EXPECT_GT(stats.trx_start_time, time_start);
  EXPECT_GT(stats.trx_end_time, time_start);
  EXPECT_GT(stats.sum_query_duration.count(), 0);
  EXPECT_GE(stats.prepared_statements_current,
            old_stats.prepared_statements_current);
}

UTEST_F(PostgreStats, TransactionMultiExecutions) {
  const auto exec_count = 10;
  [[maybe_unused]] const auto old_stats = conn->GetStatsAndReset();

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
  EXPECT_EQ(0, stats.portal_bind_total);
  EXPECT_EQ(0, stats.error_execute_total);
}

UTEST_F(PostgreStats, SingleQuery) {
  [[maybe_unused]] const auto old_stats = conn->GetStatsAndReset();

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
  EXPECT_EQ(0, stats.portal_bind_total);
  EXPECT_EQ(0, stats.error_execute_total);
}

UTEST_F(PostgreStats, PortalExecuted) {
  const auto old_stats = conn->GetStatsAndReset();

  const auto time_start = pg::detail::SteadyClock::now();
  EXPECT_NO_THROW(
      conn->Begin(pg::TransactionOptions{}, pg::detail::SteadyClock::now()));

  pg::detail::Connection::StatementId stmt_id;
  EXPECT_NO_THROW(stmt_id = conn->PortalBind("select 1", "test", {}, {}));
  EXPECT_NO_THROW(conn->PortalExecute(stmt_id, "test", 0, {}));
  EXPECT_NO_THROW(conn->Commit());

  const auto stats = conn->GetStatsAndReset();
  EXPECT_EQ(1, stats.trx_total);
  EXPECT_EQ(1, stats.commit_total);
  EXPECT_EQ(0, stats.rollback_total);
  EXPECT_EQ(0, stats.out_of_trx);
  EXPECT_EQ(1, stats.parse_total);
  EXPECT_EQ(3, stats.execute_total);
  EXPECT_EQ(1, stats.reply_total);
  EXPECT_EQ(1, stats.portal_bind_total);
  EXPECT_EQ(0, stats.error_execute_total);
  EXPECT_GT(stats.trx_start_time, time_start);
  EXPECT_GT(stats.trx_end_time, time_start);
  EXPECT_GT(stats.sum_query_duration.count(), 0);
  EXPECT_GT(stats.prepared_statements_current,
            old_stats.prepared_statements_current);
}

UTEST_F(PostgreStats, PortalInvalidQuery) {
  const auto old_stats = conn->GetStatsAndReset();

  EXPECT_NO_THROW(
      conn->Begin(pg::TransactionOptions{}, pg::detail::SteadyClock::now()));
  EXPECT_THROW(conn->PortalBind("invalid query", "test", {}, {}),
               pg::SyntaxError);
  EXPECT_NO_THROW(conn->Rollback());

  const auto stats = conn->GetStatsAndReset();
  EXPECT_EQ(1, stats.trx_total);
  EXPECT_EQ(0, stats.commit_total);
  EXPECT_EQ(1, stats.rollback_total);
  EXPECT_EQ(0, stats.out_of_trx);
  EXPECT_EQ(0, stats.parse_total);
  EXPECT_EQ(2, stats.execute_total);
  EXPECT_EQ(0, stats.reply_total);
  EXPECT_EQ(1, stats.portal_bind_total);
  EXPECT_EQ(1, stats.error_execute_total);
  EXPECT_GT(stats.sum_query_duration.count(), 0);
  EXPECT_GE(stats.prepared_statements_current,
            old_stats.prepared_statements_current);
}

UTEST_F(PostgreStats, PortalDuplicateName) {
  const auto old_stats = conn->GetStatsAndReset();

  EXPECT_NO_THROW(
      conn->Begin(pg::TransactionOptions{}, pg::detail::SteadyClock::now()));
  EXPECT_NO_THROW(conn->PortalBind("select 1", "test", {}, {}));
  EXPECT_THROW(conn->PortalBind("select 2", "test", {}, {}),
               pg::AccessRuleViolation);
  EXPECT_NO_THROW(conn->Rollback());

  const auto stats = conn->GetStatsAndReset();
  EXPECT_EQ(1, stats.trx_total);
  EXPECT_EQ(0, stats.commit_total);
  EXPECT_EQ(1, stats.rollback_total);
  EXPECT_EQ(0, stats.out_of_trx);
  EXPECT_EQ(2, stats.parse_total);
  EXPECT_EQ(2, stats.execute_total);
  EXPECT_EQ(0, stats.reply_total);
  EXPECT_EQ(2, stats.portal_bind_total);
  EXPECT_EQ(1, stats.error_execute_total);
  EXPECT_GT(stats.sum_query_duration.count(), 0);
  EXPECT_GE(stats.prepared_statements_current,
            old_stats.prepared_statements_current);
}

}  // namespace

USERVER_NAMESPACE_END
