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
    EXPECT_EQ(stats.connection.open_total, 0);
    EXPECT_EQ(stats.connection.drop_total, 0);
    EXPECT_EQ(stats.connection.active, 0);
    EXPECT_EQ(stats.connection.used, 0);
    EXPECT_EQ(stats.connection.maximum, 10);
    EXPECT_EQ(stats.transaction.total, 0);
    EXPECT_EQ(stats.transaction.commit_total, 0);
    EXPECT_EQ(stats.transaction.rollback_total, 0);
    EXPECT_EQ(stats.transaction.parse_total, 0);
    EXPECT_EQ(stats.transaction.execute_total, 0);
    EXPECT_EQ(stats.transaction.reply_total, 0);
    EXPECT_EQ(stats.transaction.bin_reply_total, 0);
    EXPECT_EQ(stats.transaction.error_execute_total, 0);
    EXPECT_EQ(stats.connection.error_total, 0);
    EXPECT_EQ(stats.pool_error_exhaust_total, 0);
    EXPECT_EQ(
        stats.transaction.total_percentile.GetCurrentCounter().GetPercentile(
            100),
        0);
    EXPECT_EQ(
        stats.transaction.busy_percentile.GetCurrentCounter().GetPercentile(
            100),
        0);
  });
}

TEST_P(PostgrePoolStats, MinPoolSize) {
  RunInCoro([this] {
    const auto min_pool_size = 2;
    pg::ConnectionPool pool(dsn_, GetTaskProcessor(), min_pool_size, 10);

    // We can't check all the counters as some of them are used for internal ops
    const auto& stats = pool.GetStatistics();
    EXPECT_LE(stats.connection.open_total, min_pool_size);
    EXPECT_EQ(stats.connection.drop_total, 0);
    EXPECT_LE(stats.connection.active, min_pool_size);
    EXPECT_EQ(stats.connection.used, 0);
    EXPECT_EQ(stats.connection.maximum, 10);
    EXPECT_EQ(stats.transaction.total, 0);
    EXPECT_EQ(stats.transaction.commit_total, 0);
    EXPECT_EQ(stats.transaction.rollback_total, 0);
    EXPECT_EQ(stats.transaction.error_execute_total, 0);
    EXPECT_EQ(stats.connection.error_total, 0);
    EXPECT_EQ(stats.pool_error_exhaust_total, 0);
    EXPECT_EQ(
        stats.transaction.total_percentile.GetCurrentCounter().GetPercentile(
            100),
        0);
    EXPECT_EQ(
        stats.transaction.busy_percentile.GetCurrentCounter().GetPercentile(
            100),
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
    EXPECT_GE(stats.connection.open_total, 1);
    EXPECT_EQ(stats.connection.drop_total, 0);
    EXPECT_GE(stats.connection.active, 1);
    EXPECT_EQ(stats.connection.used, 0);
    EXPECT_EQ(stats.connection.maximum, 10);
    EXPECT_EQ(stats.transaction.total, trx_count);
    EXPECT_EQ(stats.transaction.commit_total, trx_count);
    EXPECT_EQ(stats.transaction.rollback_total, 0);
    EXPECT_GE(stats.transaction.parse_total, 1);
    EXPECT_EQ(stats.transaction.execute_total, query_exec_count);
    EXPECT_EQ(stats.transaction.reply_total, trx_count * exec_count);
    EXPECT_EQ(stats.transaction.bin_reply_total, trx_count * exec_count);
    EXPECT_EQ(stats.transaction.error_execute_total, 0);
    EXPECT_EQ(stats.connection.error_total, 0);
    EXPECT_EQ(stats.pool_error_exhaust_total, 0);
    EXPECT_EQ(
        stats.transaction.total_percentile.GetStatsForPeriod(duration_min, true)
            .Count(),
        trx_count);
    EXPECT_EQ(
        stats.transaction.busy_percentile.GetStatsForPeriod(duration_min, true)
            .Count(),
        trx_count);
  });
}

TEST_P(PostgrePoolStats, ConnUsed) {
  RunInCoro([this] {
    pg::ConnectionPool pool(dsn_, GetTaskProcessor(), 1, 10);
    pg::detail::ConnectionPtr conn;

    EXPECT_NO_THROW(conn = pool.GetConnection())
        << "Obtained connection from pool";

    const auto& stats = pool.GetStatistics();
    EXPECT_EQ(stats.connection.used, 1);
  });
}

}  // namespace
