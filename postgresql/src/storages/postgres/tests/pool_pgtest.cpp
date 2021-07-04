#include <storages/postgres/tests/util_pgtest.hpp>

#include <utest/utest.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/sleep.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/pool.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/exceptions.hpp>

namespace pg = storages::postgres;

namespace {

void PoolTransaction(const std::shared_ptr<pg::detail::ConnectionPool>& pool) {
  pg::Transaction trx{pg::detail::ConnectionPtr(nullptr)};
  pg::ResultSet res{nullptr};

  // TODO Check idle connection count before and after begin
  EXPECT_NO_THROW(trx = pool->Begin(pg::TransactionOptions{}));
  EXPECT_NO_THROW(res = trx.Execute("select 1"));
  EXPECT_FALSE(res.IsEmpty()) << "Result set is obtained";
  // TODO Check idle connection count before and after commit
  EXPECT_NO_THROW(trx.Commit());
  EXPECT_THROW(trx.Commit(), pg::NotInTransaction);
  EXPECT_NO_THROW(trx.Rollback());
}

}  // namespace

namespace storages::postgres {

static void PrintTo(const CommandControl& cmd_ctl, std::ostream* os) {
  *os << "CommandControl{execute=" << cmd_ctl.execute.count()
      << ", statement=" << cmd_ctl.statement.count() << '}';
}

}  // namespace storages::postgres

class PostgrePool : public PostgreSQLBase,
                    public ::testing::WithParamInterface<pg::Dsn> {};

INSTANTIATE_UTEST_SUITE_P(/*empty*/, PostgrePool,
                          ::testing::ValuesIn(GetDsnFromEnv()), DsnToString);

UTEST_P(PostgrePool, ConnectionPool) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetParam(), GetTaskProcessor(), {1, 10, 10}, kCachePreparedStatements,
      GetTestCmdCtls(), {}, {});
  pg::detail::ConnectionPtr conn(nullptr);

  EXPECT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";
  CheckConnection(std::move(conn));
}

UTEST_P(PostgrePool, ConnectionPoolInitiallyEmpty) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetParam(), GetTaskProcessor(), {0, 1, 10}, kCachePreparedStatements,
      GetTestCmdCtls(), {}, {});
  pg::detail::ConnectionPtr conn(nullptr);

  EXPECT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from empty pool";
  CheckConnection(std::move(conn));
}

UTEST_P(PostgrePool, ConnectionPoolReachedMaxSize) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetParam(), GetTaskProcessor(), {1, 1, 10}, kCachePreparedStatements,
      GetTestCmdCtls(), {}, {});
  pg::detail::ConnectionPtr conn(nullptr);

  EXPECT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";
  EXPECT_THROW(pg::detail::ConnectionPtr conn2 = pool->Acquire(MakeDeadline()),
               pg::PoolError)
      << "Pool reached max size";

  CheckConnection(std::move(conn));
}

UTEST_P(PostgrePool, BlockWaitingOnAvailableConnection) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetParam(), GetTaskProcessor(), {1, 1, 10}, kCachePreparedStatements,
      GetTestCmdCtls(), {}, {});
  pg::detail::ConnectionPtr conn(nullptr);

  EXPECT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";
  // Free up connection asynchronously
  engine::impl::Async(
      GetTaskProcessor(),
      [](pg::detail::ConnectionPtr conn) {
        conn = pg::detail::ConnectionPtr(nullptr);
      },
      std::move(conn))
      .Detach();
  EXPECT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Execution blocked because pool reached max size, but connection "
         "found later";

  CheckConnection(std::move(conn));
}

UTEST_P(PostgrePool, PoolInitialSizeExceedMaxSize) {
  EXPECT_THROW(pg::detail::ConnectionPool::Create(
                   GetParam(), GetTaskProcessor(), {2, 1, 10},
                   kCachePreparedStatements, GetTestCmdCtls(), {}, {}),
               pg::InvalidConfig)
      << "Pool reached max size";
}

UTEST_P(PostgrePool, PoolTransaction) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetParam(), GetTaskProcessor(), {1, 10, 10}, kCachePreparedStatements,
      GetTestCmdCtls(), {}, {});
  PoolTransaction(pool);
}

UTEST_P(PostgrePool, PoolAliveIfConnectionExists) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetParam(), GetTaskProcessor(), pg::PoolSettings{1, 1, 10},
      kCachePreparedStatements, GetTestCmdCtls(), testsuite::PostgresControl{},
      error_injection::Settings{});
  pg::detail::ConnectionPtr conn(nullptr);

  EXPECT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";
  pool.reset();
  CheckConnection(std::move(conn));
}

UTEST_P(PostgrePool, ConnectionPtrWorks) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetParam(), GetTaskProcessor(), pg::PoolSettings{2, 2, 10},
      kCachePreparedStatements, GetTestCmdCtls(), testsuite::PostgresControl{},
      error_injection::Settings{});
  pg::detail::ConnectionPtr conn(nullptr);

  EXPECT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";
  EXPECT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained another connection from pool";
  CheckConnection(std::move(conn));

  // We still should have initial count of working connections in the pool
  EXPECT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool again";
  EXPECT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained another connection from pool again";
  pg::detail::ConnectionPtr conn2(nullptr);
  EXPECT_NO_THROW(conn2 = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool one more time";
  pool.reset();
  CheckConnection(std::move(conn));
  CheckConnection(std::move(conn2));
}

UTEST_P(PostgrePool, AsyncMinPool) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetParam(), GetTaskProcessor(), pg::PoolSettings{1, 1, 10},
      kCachePreparedStatements, GetTestCmdCtls(), testsuite::PostgresControl{},
      error_injection::Settings{});
  const auto& stats = pool->GetStatistics();
  EXPECT_EQ(0, stats.connection.open_total);
  EXPECT_EQ(1, stats.connection.active);
}

UTEST_P(PostgrePool, SyncMinPool) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetParam(), GetTaskProcessor(),
      pg::PoolSettings{1, 1, 10, /* sync_start */ true},
      kCachePreparedStatements, GetTestCmdCtls(), testsuite::PostgresControl{},
      error_injection::Settings{});

  const auto& stats = pool->GetStatistics();
  EXPECT_EQ(1, stats.connection.open_total);
  EXPECT_EQ(1, stats.connection.active);
}

UTEST_P(PostgrePool, ConnectionCleanup) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetParam(), GetTaskProcessor(), pg::PoolSettings{1, 1, 10},
      kCachePreparedStatements,
      storages::postgres::DefaultCommandControls(
          pg::CommandControl{std::chrono::milliseconds{100},
                             std::chrono::seconds{1}},
          {}, {}),
      testsuite::PostgresControl{}, error_injection::Settings{});

  {
    const auto& stats = pool->GetStatistics();
    EXPECT_EQ(0, stats.connection.open_total);
    EXPECT_EQ(1, stats.connection.active);
    EXPECT_EQ(0, stats.connection.error_total);

    pg::Transaction trx{pg::detail::ConnectionPtr(nullptr)};
    EXPECT_NO_THROW(trx = pool->Begin({})) << "Start transaction in a pool";

    EXPECT_EQ(1, stats.connection.open_total);
    EXPECT_EQ(1, stats.connection.active);
    EXPECT_EQ(1, stats.connection.used);
    EXPECT_THROW(trx.Execute("select pg_sleep(1)"), pg::ConnectionTimeoutError)
        << "Fail statement on timeout";
    EXPECT_ANY_THROW(trx.Commit()) << "Connection is left in an unusable state";
  }
  {
    const auto& stats = pool->GetStatistics();
    EXPECT_EQ(1, stats.connection.open_total);
    EXPECT_EQ(1, stats.connection.active);
    EXPECT_EQ(1, stats.connection.used);
    EXPECT_EQ(0, stats.connection.drop_total);
    EXPECT_EQ(0, stats.connection.error_total);
  }
}

UTEST_P(PostgrePool, QueryCancel) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetParam(), GetTaskProcessor(), pg::PoolSettings{1, 1, 10},
      kCachePreparedStatements,
      storages::postgres::DefaultCommandControls(
          pg::CommandControl{std::chrono::milliseconds{100},
                             std::chrono::milliseconds{10}},
          {}, {}),
      testsuite::PostgresControl{}, error_injection::Settings{});
  {
    pg::Transaction trx{pg::detail::ConnectionPtr(nullptr)};
    EXPECT_NO_THROW(trx = pool->Begin({})) << "Start transaction in a pool";

    EXPECT_THROW(trx.Execute("select pg_sleep(1)"), pg::QueryCancelled)
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
}

UTEST_P(PostgrePool, DefaultCmdCtl) {
  using Source = pg::detail::DefaultCommandControlSource;
  const pg::CommandControl custom_cmd_ctl{std::chrono::seconds{2},
                                          std::chrono::seconds{1}};

  storages::postgres::DefaultCommandControls default_cmd_ctls(GetTestCmdCtls());

  auto pool = pg::detail::ConnectionPool::Create(
      GetParam(), GetTaskProcessor(), {1, 1, 10}, kCachePreparedStatements,
      default_cmd_ctls, {}, {});

  EXPECT_EQ(kTestCmdCtl, pool->GetDefaultCommandControl());

  default_cmd_ctls.UpdateDefaultCmdCtl(kTestCmdCtl, Source::kGlobalConfig);
  EXPECT_EQ(kTestCmdCtl, pool->GetDefaultCommandControl());

  default_cmd_ctls.UpdateDefaultCmdCtl(custom_cmd_ctl, Source::kGlobalConfig);
  EXPECT_EQ(custom_cmd_ctl, pool->GetDefaultCommandControl());

  default_cmd_ctls.UpdateDefaultCmdCtl(kTestCmdCtl, Source::kGlobalConfig);
  EXPECT_EQ(kTestCmdCtl, pool->GetDefaultCommandControl());

  default_cmd_ctls.UpdateDefaultCmdCtl(custom_cmd_ctl, Source::kGlobalConfig);
  EXPECT_EQ(custom_cmd_ctl, pool->GetDefaultCommandControl());

  // after this, global config should be ignored
  default_cmd_ctls.UpdateDefaultCmdCtl(kTestCmdCtl, Source::kUser);
  EXPECT_EQ(kTestCmdCtl, pool->GetDefaultCommandControl());

  default_cmd_ctls.UpdateDefaultCmdCtl(custom_cmd_ctl, Source::kGlobalConfig);
  EXPECT_EQ(kTestCmdCtl, pool->GetDefaultCommandControl());

  default_cmd_ctls.UpdateDefaultCmdCtl(custom_cmd_ctl, Source::kUser);
  EXPECT_EQ(custom_cmd_ctl, pool->GetDefaultCommandControl());

  default_cmd_ctls.UpdateDefaultCmdCtl(kTestCmdCtl, Source::kGlobalConfig);
  EXPECT_EQ(custom_cmd_ctl, pool->GetDefaultCommandControl());

  default_cmd_ctls.UpdateDefaultCmdCtl(kTestCmdCtl, Source::kUser);
  EXPECT_EQ(kTestCmdCtl, pool->GetDefaultCommandControl());
}
