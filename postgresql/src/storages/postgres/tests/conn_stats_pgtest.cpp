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

INSTANTIATE_UTEST_SUITE_P(
    StatsSettings, PostgreStats,
    ::testing::Values(kCachePreparedStatements, kPipelineEnabled),
    [](const testing::TestParamInfo<PostgreStats::ParamType>& info) {
      if (info.param.pipeline_mode == pg::PipelineMode::kEnabled) {
        return "PipelineEnabled";
      } else {
        return "PipelineDisabled";
      }
    });

UTEST_P(PostgreStats, NoTransactions) {
  // We can't check all the counters as some of them are used for internal ops
  const auto stats = GetConn()->GetStatsAndReset();
  EXPECT_EQ(0, stats.trx_total);
  EXPECT_EQ(0, stats.commit_total);
  EXPECT_EQ(0, stats.rollback_total);
  EXPECT_EQ(0, stats.out_of_trx);
  EXPECT_EQ(0, stats.portal_bind_total);
  EXPECT_EQ(0, stats.error_execute_total);
  EXPECT_EQ(0, stats.trx_start_time.time_since_epoch().count());
  EXPECT_EQ(0, stats.trx_end_time.time_since_epoch().count());
}

UTEST_P(PostgreStats, StatsResetAfterGet) {
  [[maybe_unused]] const auto old_stats = GetConn()->GetStatsAndReset();

  const auto stats = GetConn()->GetStatsAndReset();
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

UTEST_P(PostgreStats, TransactionExecuted) {
  const auto old_stats = GetConn()->GetStatsAndReset();

  const auto time_start = pg::detail::SteadyClock::now();
  UEXPECT_NO_THROW(GetConn()->Begin(pg::TransactionOptions{},
                                    pg::detail::SteadyClock::now()));
  UEXPECT_NO_THROW(GetConn()->Execute("select 1"))
      << "select 1 successfully executed";
  UEXPECT_NO_THROW(GetConn()->Commit());

  const auto stats = GetConn()->GetStatsAndReset();
  EXPECT_EQ(1, stats.trx_total);
  EXPECT_EQ(1, stats.commit_total);
  EXPECT_EQ(0, stats.rollback_total);
  EXPECT_EQ(0, stats.out_of_trx);
  EXPECT_EQ(1, stats.parse_total);
  EXPECT_EQ(3, stats.execute_total);
  EXPECT_EQ(1, stats.reply_total);
  EXPECT_EQ(0, stats.portal_bind_total);
  EXPECT_EQ(0, stats.error_execute_total);
  EXPECT_GE(stats.trx_start_time, time_start);
  EXPECT_GT(stats.trx_end_time, time_start);
  EXPECT_GT(stats.sum_query_duration.count(), 0);
  EXPECT_GT(stats.prepared_statements_current,
            old_stats.prepared_statements_current);
}

UTEST_P(PostgreStats, TransactionFailed) {
  const auto old_stats = GetConn()->GetStatsAndReset();

  const auto time_start = pg::detail::SteadyClock::now();
  UEXPECT_NO_THROW(GetConn()->Begin(pg::TransactionOptions{},
                                    pg::detail::SteadyClock::now()));
  UEXPECT_THROW(GetConn()->Execute("select select select"), pg::Error)
      << "invalid query throws exception";
  UEXPECT_NO_THROW(GetConn()->Rollback());

  const auto stats = GetConn()->GetStatsAndReset();
  EXPECT_EQ(1, stats.trx_total);
  EXPECT_EQ(0, stats.commit_total);
  EXPECT_EQ(1, stats.rollback_total);
  EXPECT_EQ(0, stats.out_of_trx);
  EXPECT_EQ(0, stats.parse_total);
  EXPECT_EQ(3, stats.execute_total);
  EXPECT_EQ(0, stats.reply_total);
  EXPECT_EQ(0, stats.portal_bind_total);
  EXPECT_EQ(1, stats.error_execute_total);
  EXPECT_GE(stats.trx_start_time, time_start);
  EXPECT_GT(stats.trx_end_time, time_start);
  EXPECT_GT(stats.sum_query_duration.count(), 0);
  EXPECT_GE(stats.prepared_statements_current,
            old_stats.prepared_statements_current);
}

UTEST_P(PostgreStats, TransactionMultiExecutions) {
  const auto exec_count = 10;
  [[maybe_unused]] const auto old_stats = GetConn()->GetStatsAndReset();

  UEXPECT_NO_THROW(GetConn()->Begin(pg::TransactionOptions{},
                                    pg::detail::SteadyClock::now()));
  for (auto i = 0; i < exec_count; ++i) {
    UEXPECT_NO_THROW(GetConn()->Execute("select 1"))
        << "select 1 successfully executed";
  }
  UEXPECT_NO_THROW(GetConn()->Commit());

  const auto stats = GetConn()->GetStatsAndReset();
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

UTEST_P(PostgreStats, SingleQuery) {
  [[maybe_unused]] const auto old_stats = GetConn()->GetStatsAndReset();

  UEXPECT_NO_THROW(GetConn()->Start(pg::detail::SteadyClock::now()));
  UEXPECT_NO_THROW(GetConn()->Execute("select 1"));
  UEXPECT_NO_THROW(GetConn()->Finish());

  const auto stats = GetConn()->GetStatsAndReset();
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

UTEST_P(PostgreStats, PortalExecuted) {
  const auto old_stats = GetConn()->GetStatsAndReset();

  const auto time_start = pg::detail::SteadyClock::now();
  UEXPECT_NO_THROW(GetConn()->Begin(pg::TransactionOptions{},
                                    pg::detail::SteadyClock::now()));

  pg::detail::Connection::StatementId stmt_id;
  UEXPECT_NO_THROW(stmt_id = GetConn()->PortalBind("select 1", "test", {}, {}));
  UEXPECT_NO_THROW(GetConn()->PortalExecute(stmt_id, "test", 0, {}));
  UEXPECT_NO_THROW(GetConn()->Commit());

  const auto stats = GetConn()->GetStatsAndReset();
  EXPECT_EQ(1, stats.trx_total);
  EXPECT_EQ(1, stats.commit_total);
  EXPECT_EQ(0, stats.rollback_total);
  EXPECT_EQ(0, stats.out_of_trx);
  EXPECT_EQ(1, stats.parse_total);
  EXPECT_EQ(3, stats.execute_total);
  EXPECT_EQ(1, stats.reply_total);
  EXPECT_EQ(1, stats.portal_bind_total);
  EXPECT_EQ(0, stats.error_execute_total);
  EXPECT_GE(stats.trx_start_time, time_start);
  EXPECT_GT(stats.trx_end_time, time_start);
  EXPECT_GT(stats.sum_query_duration.count(), 0);
  EXPECT_GT(stats.prepared_statements_current,
            old_stats.prepared_statements_current);
}

UTEST_P(PostgreStats, PortalInvalidQuery) {
  const auto old_stats = GetConn()->GetStatsAndReset();

  UEXPECT_NO_THROW(GetConn()->Begin(pg::TransactionOptions{},
                                    pg::detail::SteadyClock::now()));
  UEXPECT_THROW(GetConn()->PortalBind("invalid query", "test", {}, {}),
                pg::SyntaxError);
  UEXPECT_NO_THROW(GetConn()->Rollback());

  const auto stats = GetConn()->GetStatsAndReset();
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

UTEST_P(PostgreStats, PortalDuplicateName) {
  const auto old_stats = GetConn()->GetStatsAndReset();

  UEXPECT_NO_THROW(GetConn()->Begin(pg::TransactionOptions{},
                                    pg::detail::SteadyClock::now()));
  UEXPECT_NO_THROW(GetConn()->PortalBind("select 1", "test", {}, {}));
  UEXPECT_THROW(GetConn()->PortalBind("select 2", "test", {}, {}),
                pg::AccessRuleViolation);
  UEXPECT_NO_THROW(GetConn()->Rollback());

  const auto stats = GetConn()->GetStatsAndReset();
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
