#include <storages/postgres/tests/util_test.hpp>

#include <gtest/gtest.h>

#include <engine/async.hpp>
#include <engine/standalone.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/pool.hpp>

namespace pg = storages::postgres;

namespace {

class PostgrePoolStats : public PostgreSQLBase,
                         public ::testing::WithParamInterface<std::string> {
  void ReadParam() override { dsn_ = GetParam(); }

 protected:
  std::string dsn_;
};

INSTANTIATE_TEST_CASE_P(/*empty*/, PostgrePoolStats,
                        ::testing::ValuesIn(GetDsnFromEnv()), DsnToString);

TEST_P(PostgrePoolStats, EmptyPool) {
  RunInCoro([this] {
    pg::ConnectionPool pool(dsn_, GetTaskProcessor(), 0, 10);

    const auto& stats = pool.GetStatistics();
    EXPECT_EQ(stats.conn_open_total, 0);
    EXPECT_EQ(stats.conn_drop_total, 0);
    EXPECT_EQ(stats.trx_total, 0);
    EXPECT_EQ(stats.trx_commit_total, 0);
    EXPECT_EQ(stats.trx_rollback_total, 0);
    EXPECT_EQ(stats.trx_parse_total, 0);
    EXPECT_EQ(stats.trx_execute_total, 0);
    EXPECT_EQ(stats.trx_reply_total, 0);
    EXPECT_EQ(stats.trx_bin_reply_total, 0);
    EXPECT_EQ(stats.trx_error_execute_total, 0);
    EXPECT_EQ(stats.conn_error_total, 0);
    EXPECT_EQ(stats.pool_error_exhaust_total, 0);
    EXPECT_EQ(stats.trx_exec_percentile.GetCurrentCounter().GetPercentile(100),
              0);
    EXPECT_EQ(stats.trx_delay_percentile.GetCurrentCounter().GetPercentile(100),
              0);
    EXPECT_EQ(stats.trx_user_percentile.GetCurrentCounter().GetPercentile(100),
              0);
  });
}

TEST_P(PostgrePoolStats, MinPoolSize) {
  RunInCoro([this] {
    const auto min_pool_size = 2;
    pg::ConnectionPool pool(dsn_, GetTaskProcessor(), min_pool_size, 10);

    // We can't check all the counters as some of them are used for internal ops
    const auto& stats = pool.GetStatistics();
    EXPECT_EQ(stats.conn_open_total, min_pool_size);
    EXPECT_EQ(stats.conn_drop_total, 0);
    EXPECT_EQ(stats.trx_total, 0);
    EXPECT_EQ(stats.trx_commit_total, 0);
    EXPECT_EQ(stats.trx_rollback_total, 0);
    EXPECT_EQ(stats.trx_error_execute_total, 0);
    EXPECT_EQ(stats.conn_error_total, 0);
    EXPECT_EQ(stats.pool_error_exhaust_total, 0);
    EXPECT_EQ(stats.trx_exec_percentile.GetCurrentCounter().GetPercentile(100),
              0);
    EXPECT_EQ(stats.trx_delay_percentile.GetCurrentCounter().GetPercentile(100),
              0);
    EXPECT_EQ(stats.trx_user_percentile.GetCurrentCounter().GetPercentile(100),
              0);
  });
}

TEST_P(PostgrePoolStats, RunTransactions) {
  RunInCoro([this] {
    pg::ConnectionPool pool(dsn_, GetTaskProcessor(), 1, 10);

    const auto trx_count = 5;
    const auto exec_count = 10;

    std::array<engine::TaskWithResult<void>, trx_count> tasks;
    for (auto i = 0; i < trx_count; ++i) {
      auto task = engine::Async([&pool] {
        pg::detail::ConnectionPtr conn;

        EXPECT_NO_THROW(conn = pool.GetConnection())
            << "Obtained connection from pool";
        ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

        std::ignore = conn->GetStatsAndReset();
        EXPECT_NO_THROW(conn->Begin(pg::TransactionOptions{},
                                    pg::detail::SteadyClock::now()));
        for (auto i = 0; i < exec_count; ++i) {
          EXPECT_NO_THROW(conn->Execute("select 1"))
              << "select 1 successfully executed";
        }
        EXPECT_NO_THROW(conn->Commit());
      });
      tasks[i] = std::move(task);
    }

    for (auto&& task : tasks) {
      task.Get();
    }

    const auto query_exec_count = trx_count * (exec_count + /*begin-commit*/ 2);
    const auto duration_min = pg::detail::SteadyClock::duration::min();
    const auto& stats = pool.GetStatistics();
    EXPECT_GE(stats.conn_open_total, 1);
    EXPECT_EQ(stats.conn_drop_total, 0);
    EXPECT_EQ(stats.trx_total, trx_count);
    EXPECT_EQ(stats.trx_commit_total, trx_count);
    EXPECT_EQ(stats.trx_rollback_total, 0);
    EXPECT_GE(stats.trx_parse_total, 1);
    EXPECT_EQ(stats.trx_execute_total, query_exec_count);
    EXPECT_EQ(stats.trx_reply_total, trx_count * exec_count);
    EXPECT_EQ(stats.trx_bin_reply_total, trx_count * exec_count);
    EXPECT_EQ(stats.trx_error_execute_total, 0);
    EXPECT_EQ(stats.conn_error_total, 0);
    EXPECT_EQ(stats.pool_error_exhaust_total, 0);
    EXPECT_EQ(
        stats.trx_exec_percentile.GetStatsForPeriod(duration_min, true).Count(),
        trx_count);
    EXPECT_EQ(stats.trx_delay_percentile.GetStatsForPeriod(duration_min, true)
                  .Count(),
              trx_count);
    EXPECT_EQ(
        stats.trx_user_percentile.GetStatsForPeriod(duration_min, true).Count(),
        trx_count);
  });
}

}  // namespace
