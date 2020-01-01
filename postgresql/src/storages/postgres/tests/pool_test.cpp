#include <storages/postgres/tests/util_test.hpp>

#include <gtest/gtest.h>

#include <utest/utest.hpp>

#include <engine/async.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/pool.hpp>

namespace pg = storages::postgres;

namespace {

void PoolTransaction(pg::ConnectionPool& pool) {
  pg::Transaction trx{pg::detail::ConnectionPtr(nullptr)};
  pg::ResultSet res{nullptr};

  // TODO Check idle connection count before and after begin
  EXPECT_NO_THROW(trx = pool.Begin(pg::TransactionOptions{}, MakeDeadline()));
  EXPECT_NO_THROW(res = trx.Execute("select 1"));
  EXPECT_FALSE(res.IsEmpty()) << "Result set is obtained";
  // TODO Check idle connection count before and after commit
  EXPECT_NO_THROW(trx.Commit());
  EXPECT_THROW(trx.Commit(), pg::NotInTransaction);
  EXPECT_NO_THROW(trx.Rollback());
}

}  // namespace

class PostgrePool : public PostgreSQLBase,
                    public ::testing::WithParamInterface<std::string> {
  void ReadParam() override { dsn_ = GetParam(); }

 protected:
  std::string dsn_;
};

INSTANTIATE_TEST_CASE_P(/*empty*/, PostgrePool,
                        ::testing::ValuesIn(GetDsnFromEnv()), DsnToString);

TEST_P(PostgrePool, ConnectionPool) {
  RunInCoro([this] {
    pg::ConnectionPool pool(dsn_, GetTaskProcessor(), {1, 10, 10},
                            kCachePreparedStatements, kTestCmdCtl, {});
    pg::detail::ConnectionPtr conn(nullptr);

    EXPECT_NO_THROW(conn = pool.GetConnection(MakeDeadline()))
        << "Obtained connection from pool";
    CheckConnection(std::move(conn));
  });
}

TEST_P(PostgrePool, ConnectionPoolInitiallyEmpty) {
  RunInCoro([this] {
    pg::ConnectionPool pool(dsn_, GetTaskProcessor(), {0, 1, 10},
                            kCachePreparedStatements, kTestCmdCtl, {});
    pg::detail::ConnectionPtr conn(nullptr);

    EXPECT_NO_THROW(conn = pool.GetConnection(MakeDeadline()))
        << "Obtained connection from empty pool";
    CheckConnection(std::move(conn));
  });
}

TEST_P(PostgrePool, ConnectionPoolReachedMaxSize) {
  RunInCoro([this] {
    pg::ConnectionPool pool(dsn_, GetTaskProcessor(), {1, 1, 10},
                            kCachePreparedStatements, kTestCmdCtl, {});
    pg::detail::ConnectionPtr conn(nullptr);

    EXPECT_NO_THROW(conn = pool.GetConnection(MakeDeadline()))
        << "Obtained connection from pool";
    EXPECT_THROW(
        pg::detail::ConnectionPtr conn2 = pool.GetConnection(MakeDeadline()),
        pg::PoolError)
        << "Pool reached max size";

    CheckConnection(std::move(conn));
  });
}

TEST_P(PostgrePool, BlockWaitingOnAvailableConnection) {
  RunInCoro([this] {
    pg::ConnectionPool pool(dsn_, GetTaskProcessor(), {1, 1, 10},
                            kCachePreparedStatements, kTestCmdCtl, {});
    pg::detail::ConnectionPtr conn(nullptr);

    EXPECT_NO_THROW(conn = pool.GetConnection(MakeDeadline()))
        << "Obtained connection from pool";
    // Free up connection asynchronously
    engine::impl::Async(GetTaskProcessor(),
                        [](pg::detail::ConnectionPtr conn) {
                          conn = pg::detail::ConnectionPtr(nullptr);
                        },
                        std::move(conn))
        .Detach();
    EXPECT_NO_THROW(conn = pool.GetConnection(MakeDeadline()))
        << "Execution blocked because pool reached max size, but connection "
           "found later";

    CheckConnection(std::move(conn));
  });
}

TEST_P(PostgrePool, PoolInitialSizeExceedMaxSize) {
  RunInCoro([this] {
    EXPECT_THROW(pg::ConnectionPool(dsn_, GetTaskProcessor(), {2, 1, 10},
                                    kCachePreparedStatements, kTestCmdCtl, {}),
                 pg::InvalidConfig)
        << "Pool reached max size";
  });
}

TEST_P(PostgrePool, PoolTransaction) {
  RunInCoro([this] {
    pg::ConnectionPool pool(dsn_, GetTaskProcessor(), {1, 10, 10},
                            kCachePreparedStatements, kTestCmdCtl, {});
    PoolTransaction(pool);
  });
}

TEST_P(PostgrePool, PoolAliveIfConnectionExists) {
  RunInCoro([this] {
    auto pool = std::make_unique<pg::ConnectionPool>(
        dsn_, GetTaskProcessor(), pg::PoolSettings{1, 1, 10},
        kCachePreparedStatements, kTestCmdCtl, error_injection::Settings{});
    pg::detail::ConnectionPtr conn(nullptr);

    EXPECT_NO_THROW(conn = pool->GetConnection(MakeDeadline()))
        << "Obtained connection from pool";
    pool.reset();
    CheckConnection(std::move(conn));
  });
}

TEST_P(PostgrePool, ConnectionPtrWorks) {
  RunInCoro([this] {
    auto pool = std::make_unique<pg::ConnectionPool>(
        dsn_, GetTaskProcessor(), pg::PoolSettings{2, 2, 10},
        kCachePreparedStatements, kTestCmdCtl, error_injection::Settings{});
    pg::detail::ConnectionPtr conn(nullptr);

    EXPECT_NO_THROW(conn = pool->GetConnection(MakeDeadline()))
        << "Obtained connection from pool";
    EXPECT_NO_THROW(conn = pool->GetConnection(MakeDeadline()))
        << "Obtained another connection from pool";
    CheckConnection(std::move(conn));

    // We still should have initial count of working connections in the pool
    EXPECT_NO_THROW(conn = pool->GetConnection(MakeDeadline()))
        << "Obtained connection from pool again";
    EXPECT_NO_THROW(conn = pool->GetConnection(MakeDeadline()))
        << "Obtained another connection from pool again";
    pg::detail::ConnectionPtr conn2(nullptr);
    EXPECT_NO_THROW(conn2 = pool->GetConnection(MakeDeadline()))
        << "Obtained connection from pool one more time";
    pool.reset();
    CheckConnection(std::move(conn));
    CheckConnection(std::move(conn2));
  });
}

TEST_P(PostgrePool, AsyncMinPool) {
  RunInCoro([this] {
    auto pool = std::make_unique<pg::ConnectionPool>(
        dsn_, GetTaskProcessor(), pg::PoolSettings{1, 1, 10},
        kCachePreparedStatements, kTestCmdCtl, error_injection::Settings{});
    const auto& stats = pool->GetStatistics();
    EXPECT_EQ(0, stats.connection.open_total);
    EXPECT_EQ(1, stats.connection.active);
  });
}

TEST_P(PostgrePool, SyncMinPool) {
  RunInCoro([this] {
    auto pool = std::make_unique<pg::ConnectionPool>(
        dsn_, GetTaskProcessor(),
        pg::PoolSettings{1, 1, 10, /* sync_start */ true},
        kCachePreparedStatements, kTestCmdCtl, error_injection::Settings{});

    const auto& stats = pool->GetStatistics();
    EXPECT_EQ(1, stats.connection.open_total);
    EXPECT_EQ(1, stats.connection.active);
  });
}

TEST_P(PostgrePool, ConnectionCleanup) {
  RunInCoro([this] {
    auto pool = std::make_unique<pg::ConnectionPool>(
        dsn_, GetTaskProcessor(), pg::PoolSettings{1, 1, 10},
        kCachePreparedStatements,
        pg::CommandControl{pg::TimeoutDuration{100}, pg::TimeoutDuration{1000}},
        error_injection::Settings{});

    {
      const auto& stats = pool->GetStatistics();
      EXPECT_EQ(0, stats.connection.open_total);
      EXPECT_EQ(1, stats.connection.active);
      EXPECT_EQ(0, stats.connection.error_total);

      pg::Transaction trx{pg::detail::ConnectionPtr(nullptr)};
      EXPECT_NO_THROW(trx = pool->Begin({}, MakeDeadline()))
          << "Start transaction in a pool";

      EXPECT_EQ(1, stats.connection.open_total);
      EXPECT_EQ(1, stats.connection.active);
      EXPECT_EQ(1, stats.connection.used);
      EXPECT_THROW(trx.Execute("select pg_sleep(1)"),
                   pg::ConnectionTimeoutError)
          << "Fail statement on timeout";
      EXPECT_ANY_THROW(trx.Commit())
          << "Connection is left in an unusable state";
    }
    {
      const auto& stats = pool->GetStatistics();
      EXPECT_EQ(1, stats.connection.open_total);
      EXPECT_EQ(1, stats.connection.active);
      EXPECT_EQ(1, stats.connection.used);
      EXPECT_EQ(0, stats.connection.drop_total);
      EXPECT_EQ(0, stats.connection.error_total);
    }
  });
}

TEST_P(PostgrePool, QueryCancel) {
  RunInCoro([this] {
    auto pool = std::make_unique<pg::ConnectionPool>(
        dsn_, GetTaskProcessor(), pg::PoolSettings{1, 1, 10},
        kCachePreparedStatements,
        pg::CommandControl{pg::TimeoutDuration{100}, pg::TimeoutDuration{10}},
        error_injection::Settings{});
    {
      pg::Transaction trx{pg::detail::ConnectionPtr(nullptr)};
      EXPECT_NO_THROW(trx = pool->Begin({}, MakeDeadline()))
          << "Start transaction in a pool";

      EXPECT_THROW(trx.Execute("select pg_sleep(1)"), pg::QueryCanceled)
          << "Fail statement on timeout";
      EXPECT_NO_THROW(trx.Commit()) << "Connection is left in a usable state";
    }
    {
      const auto& stats = pool->GetStatistics();
      EXPECT_EQ(1, stats.connection.open_total);
      EXPECT_EQ(1, stats.connection.active);
      EXPECT_EQ(0, stats.connection.used);
      EXPECT_EQ(0, stats.connection.drop_total);
      EXPECT_EQ(0, stats.connection.error_total);
    }
  });
}
