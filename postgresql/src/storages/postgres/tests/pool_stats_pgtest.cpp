#include <storages/postgres/tests/util_pgtest.hpp>

#include <gtest/gtest.h>

#include <engine/async.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/pool.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>

namespace pg = storages::postgres;

namespace {

class PostgrePoolStats : public PostgreSQLBase,
                         public ::testing::WithParamInterface<pg::Dsn> {};

INSTANTIATE_TEST_SUITE_P(/*empty*/, PostgrePoolStats,
                         ::testing::ValuesIn(GetDsnFromEnv()), DsnToString);

TEST_P(PostgrePoolStats, EmptyPool) {
  RunInCoro([] {
    auto pool = pg::detail::ConnectionPool::Create(
        GetParam(), GetTaskProcessor(), {0, 10, 10}, kCachePreparedStatements,
        GetTestCmdCtls(), {}, {});

    const auto& stats = pool->GetStatistics();
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
    EXPECT_EQ(stats.transaction.portal_bind_total, 0);
    EXPECT_EQ(stats.transaction.error_execute_total, 0);
    EXPECT_EQ(stats.connection.error_total, 0);
    EXPECT_EQ(stats.pool_exhaust_errors, 0);
    EXPECT_EQ(stats.queue_size_errors, 0);
    EXPECT_EQ(
        stats.transaction.total_percentile.GetStatsForPeriod().GetPercentile(
            100),
        0);
    EXPECT_EQ(
        stats.transaction.busy_percentile.GetStatsForPeriod().GetPercentile(
            100),
        0);
  });
}

TEST_P(PostgrePoolStats, MinPoolSize) {
  RunInCoro([] {
    const auto min_pool_size = 2;
    auto pool = pg::detail::ConnectionPool::Create(
        GetParam(), GetTaskProcessor(), {min_pool_size, 10, 10},
        kCachePreparedStatements, GetTestCmdCtls(), {}, {});

    // We can't check all the counters as some of them are used for internal ops
    const auto& stats = pool->GetStatistics();
    EXPECT_LE(stats.connection.open_total, min_pool_size);
    EXPECT_EQ(stats.connection.drop_total, 0);
    EXPECT_LE(stats.connection.active, min_pool_size);
    EXPECT_EQ(stats.connection.used, 0);
    EXPECT_EQ(stats.connection.maximum, 10);
    EXPECT_EQ(stats.transaction.total, 0);
    EXPECT_EQ(stats.transaction.commit_total, 0);
    EXPECT_EQ(stats.transaction.rollback_total, 0);
    EXPECT_EQ(stats.transaction.portal_bind_total, 0);
    EXPECT_EQ(stats.transaction.error_execute_total, 0);
    EXPECT_EQ(stats.connection.error_total, 0);
    EXPECT_EQ(stats.pool_exhaust_errors, 0);
    EXPECT_EQ(stats.queue_size_errors, 0);
    EXPECT_EQ(
        stats.transaction.total_percentile.GetStatsForPeriod().GetPercentile(
            100),
        0);
    EXPECT_EQ(
        stats.transaction.busy_percentile.GetStatsForPeriod().GetPercentile(
            100),
        0);
  });
}

TEST_P(PostgrePoolStats, RunTransactions) {
  RunInCoro([] {
    auto pool = pg::detail::ConnectionPool::Create(
        GetParam(), GetTaskProcessor(), {1, 10, 10}, kCachePreparedStatements,
        GetTestCmdCtls(), {}, {});

    const auto trx_count = 5;
    const auto exec_count = 10;

    std::vector<engine::TaskWithResult<void>> tasks;
    for (auto i = 0; i < trx_count; ++i) {
      tasks.push_back(engine::impl::Async([&pool] {
        pg::detail::ConnectionPtr conn(nullptr);

        EXPECT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
            << "Obtained connection from pool";
        ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

        [[maybe_unused]] const auto old_stats = conn->GetStatsAndReset();
        EXPECT_NO_THROW(conn->Begin(pg::TransactionOptions{},
                                    pg::detail::SteadyClock::now()));
        for (auto i = 0; i < exec_count; ++i) {
          EXPECT_NO_THROW(conn->Execute("select 1"))
              << "select 1 successfully executed";
        }
        EXPECT_NO_THROW(conn->Commit());
      }));
    }

    for (auto&& task : tasks) {
      task.Get();
    }

    const auto query_exec_count = trx_count * (exec_count + /*begin-commit*/ 2);
    const auto kDurationMin = pg::detail::SteadyClock::duration::min();
    const auto& stats = pool->GetStatistics();
    EXPECT_GE(stats.connection.open_total, 1);
    EXPECT_EQ(stats.connection.drop_total, 0);
    EXPECT_GE(stats.connection.active, 1);
    EXPECT_EQ(stats.connection.used, 0);
    EXPECT_EQ(stats.connection.maximum, 10);

    const auto prepared_stats = stats.connection.prepared_statements
                                    .GetStatsForPeriod(kDurationMin, true)
                                    .GetCurrent();
    EXPECT_GT(prepared_stats.average, 0);
    EXPECT_EQ(prepared_stats.minimum, prepared_stats.maximum);

    EXPECT_EQ(stats.transaction.total, trx_count);
    EXPECT_EQ(stats.transaction.commit_total, trx_count);
    EXPECT_EQ(stats.transaction.rollback_total, 0);
    EXPECT_GE(stats.transaction.parse_total, 1);
    EXPECT_EQ(stats.transaction.execute_total, query_exec_count);
    EXPECT_EQ(stats.transaction.reply_total, trx_count * exec_count);
    EXPECT_EQ(stats.transaction.portal_bind_total, 0);
    EXPECT_EQ(stats.transaction.error_execute_total, 0);
    EXPECT_EQ(stats.connection.error_total, 0);
    EXPECT_EQ(stats.pool_exhaust_errors, 0);
    EXPECT_EQ(stats.queue_size_errors, 0);
    EXPECT_EQ(
        stats.transaction.total_percentile.GetStatsForPeriod(kDurationMin, true)
            .Count(),
        trx_count);
    EXPECT_EQ(
        stats.transaction.busy_percentile.GetStatsForPeriod(kDurationMin, true)
            .Count(),
        trx_count);
  });
}

TEST_P(PostgrePoolStats, ConnUsed) {
  RunInCoro([] {
    auto pool = pg::detail::ConnectionPool::Create(
        GetParam(), GetTaskProcessor(), {1, 10, 10}, kCachePreparedStatements,
        GetTestCmdCtls(), {}, {});
    pg::detail::ConnectionPtr conn(nullptr);

    EXPECT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
        << "Obtained connection from pool";

    const auto& stats = pool->GetStatistics();
    EXPECT_EQ(stats.connection.used, 1);
  });
}

TEST_P(PostgrePoolStats, Portal) {
  RunInCoro([] {
    auto pool = pg::detail::ConnectionPool::Create(
        GetParam(), GetTaskProcessor(), {1, 10, 10}, kCachePreparedStatements,
        GetTestCmdCtls(), {}, {});

    {
      pg::detail::ConnectionPtr conn(nullptr);

      EXPECT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
          << "Obtained connection from pool";
      ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

      EXPECT_NO_THROW(conn->Begin(pg::TransactionOptions{},
                                  pg::detail::SteadyClock::now()));
      pg::detail::Connection::StatementId stmt_id;
      EXPECT_NO_THROW(stmt_id = conn->PortalBind("select 1", "test", {}, {}));
      pg::ResultSet res{nullptr};
      EXPECT_NO_THROW(res = conn->PortalExecute(stmt_id, "test", 0, {}));
      EXPECT_EQ(res.Size(), 1);
      EXPECT_NO_THROW(conn->Commit());
    }

    const auto& stats = pool->GetStatistics();
    EXPECT_GE(stats.connection.open_total, 1);
    EXPECT_EQ(stats.connection.drop_total, 0);
    EXPECT_GE(stats.connection.active, 1);
    EXPECT_EQ(stats.connection.used, 0);
    EXPECT_EQ(stats.connection.maximum, 10);
    EXPECT_EQ(stats.transaction.total, 1);
    EXPECT_EQ(stats.transaction.commit_total, 1);
    EXPECT_EQ(stats.transaction.rollback_total, 0);
    EXPECT_EQ(stats.transaction.parse_total, 1);
    EXPECT_EQ(stats.transaction.execute_total, 3);
    EXPECT_EQ(stats.transaction.reply_total, 1);
    EXPECT_EQ(stats.transaction.portal_bind_total, 1);
    EXPECT_EQ(stats.transaction.error_execute_total, 0);
    EXPECT_EQ(stats.connection.error_total, 0);
    EXPECT_EQ(stats.pool_exhaust_errors, 0);
    EXPECT_EQ(stats.queue_size_errors, 0);
  });
}

}  // namespace
