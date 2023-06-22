#include <storages/postgres/tests/util_pgtest.hpp>

#include <userver/utest/utest.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/sleep.hpp>

#include <engine/task/task_processor.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/pool.hpp>
#include <storages/postgres/postgres_config.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

namespace {

void PoolTransaction(const std::shared_ptr<pg::detail::ConnectionPool>& pool) {
  pg::Transaction trx{pg::detail::ConnectionPtr(nullptr)};
  pg::ResultSet res{nullptr};

  // TODO Check idle connection count before and after begin
  UEXPECT_NO_THROW(trx = pool->Begin(pg::TransactionOptions{}));
  UEXPECT_NO_THROW(res = trx.Execute("select 1"));
  EXPECT_FALSE(res.IsEmpty()) << "Result set is obtained";
  // TODO Check idle connection count before and after commit
  UEXPECT_NO_THROW(trx.Commit());
  UEXPECT_THROW(trx.Commit(), pg::NotInTransaction);
  UEXPECT_NO_THROW(trx.Rollback());
}

}  // namespace

namespace storages::postgres {

static void PrintTo(const CommandControl& cmd_ctl, std::ostream* os) {
  *os << "CommandControl{execute=" << cmd_ctl.execute.count()
      << ", statement=" << cmd_ctl.statement.count() << '}';
}

}  // namespace storages::postgres

// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class PostgrePool : public PostgreSQLBase,
                    public ::testing::WithParamInterface<pg::InitMode> {};

UTEST_P(PostgrePool, ConnectionPool) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "", GetParam(), {1, 10, 10},
      kCachePreparedStatements, {}, GetTestCmdCtls(), {}, {}, {},
      dynamic_config::GetDefaultSource());
  pg::detail::ConnectionPtr conn(nullptr);

  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";
  CheckConnection(std::move(conn));
}

UTEST_P(PostgrePool, ConnectionPoolInitiallyEmpty) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "", GetParam(), {0, 1, 10},
      kCachePreparedStatements, {}, GetTestCmdCtls(), {}, {}, {},
      dynamic_config::GetDefaultSource());
  pg::detail::ConnectionPtr conn(nullptr);

  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from empty pool";
  CheckConnection(std::move(conn));
}

UTEST_P(PostgrePool, ConnectionPoolReachedMaxSize) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "", GetParam(), {1, 1, 10},
      kCachePreparedStatements, {}, GetTestCmdCtls(), {}, {}, {},
      dynamic_config::GetDefaultSource());
  pg::detail::ConnectionPtr conn(nullptr);

  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";
  UEXPECT_THROW(pg::detail::ConnectionPtr conn2 = pool->Acquire(MakeDeadline()),
                pg::PoolError)
      << "Pool reached max size";

  CheckConnection(std::move(conn));
}

UTEST_P(PostgrePool, ConnectionPoolHighDemand) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "", GetParam(), {1, 1, 10},
      kCachePreparedStatements, {}, GetTestCmdCtls(), {}, {}, {},
      dynamic_config::GetDefaultSource());
  pg::detail::ConnectionPtr conn(nullptr);
  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";

  const auto n_tasks =
      GetTaskProcessor().GetTaskCounter().GetCreatedTasks().value;

  const auto n_acquire_tasks = 10;
  const auto n_pending_tasks = 2;
  concurrent::BackgroundTaskStorage ts{GetTaskProcessor()};
  for (auto i = 0; i < n_acquire_tasks; ++i) {
    ts.AsyncDetach("acquire", [&pool]() {
      UEXPECT_THROW(
          pg::detail::ConnectionPtr conn = pool->Acquire(MakeDeadline()),
          pg::PoolError);
    });
  }
  engine::SleepFor(std::chrono::milliseconds{100});
  ts.CancelAndWait();

  EXPECT_LE(GetTaskProcessor().GetTaskCounter().GetCreatedTasks().value,
            n_tasks + n_acquire_tasks + n_pending_tasks);

  CheckConnection(std::move(conn));
}

UTEST_P(PostgrePool, BlockWaitingOnAvailableConnection) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "", GetParam(), {1, 1, 10},
      kCachePreparedStatements, {}, GetTestCmdCtls(), {}, {}, {},
      dynamic_config::GetDefaultSource());
  pg::detail::ConnectionPtr conn(nullptr);

  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";
  // Free up connection asynchronously
  engine::AsyncNoSpan(
      GetTaskProcessor(),
      [](pg::detail::ConnectionPtr conn) {
        conn = pg::detail::ConnectionPtr(nullptr);
      },
      std::move(conn))
      .Detach();
  // NOLINTNEXTLINE(bugprone-use-after-move)
  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Execution blocked because pool reached max size, but connection "
         "found later";

  CheckConnection(std::move(conn));
}

UTEST_P(PostgrePool, PoolInitialSizeExceedMaxSize) {
  UEXPECT_THROW(
      pg::detail::ConnectionPool::Create(
          GetDsnFromEnv(), nullptr, GetTaskProcessor(), "", GetParam(),
          {2, 1, 10}, kCachePreparedStatements, {}, GetTestCmdCtls(), {}, {},
          {}, dynamic_config::GetDefaultSource()),
      pg::InvalidConfig)
      << "Pool reached max size";
}

UTEST_P(PostgrePool, PoolServerUnavailable) {
  std::shared_ptr<pg::detail::ConnectionPool> pool;
  UASSERT_NO_THROW(pool = pg::detail::ConnectionPool::Create(
                       GetUnavailableDsn(), nullptr, GetTaskProcessor(), "",
                       GetParam(), {1, 10, 10}, kCachePreparedStatements, {},
                       GetTestCmdCtls(), {}, {}, {},
                       dynamic_config::GetDefaultSource()));
  UEXPECT_THROW(pg::detail::ConnectionPtr conn = pool->Acquire(MakeDeadline()),
                pg::PoolError)
      << "Empty pool";
  const auto& stats = pool->GetStatistics();
  EXPECT_EQ(2, stats.connection.open_total);
  EXPECT_EQ(0, stats.connection.active);
  EXPECT_EQ(2, stats.connection.error_total);
}

UTEST_P(PostgrePool, PoolTransaction) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "", GetParam(), {1, 10, 10},
      kCachePreparedStatements, {}, GetTestCmdCtls(), {}, {}, {},
      dynamic_config::GetDefaultSource());
  PoolTransaction(pool);
}

UTEST_P(PostgrePool, PoolAliveIfConnectionExists) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "", GetParam(), {1, 1, 10},
      kCachePreparedStatements, {}, GetTestCmdCtls(),
      testsuite::PostgresControl{}, error_injection::Settings{}, {},
      dynamic_config::GetDefaultSource());
  pg::detail::ConnectionPtr conn(nullptr);

  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";
  pool.reset();
  CheckConnection(std::move(conn));
}

UTEST_P(PostgrePool, ConnectionPtrWorks) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "", GetParam(), {2, 2, 10},
      kCachePreparedStatements, {}, GetTestCmdCtls(),
      testsuite::PostgresControl{}, error_injection::Settings{}, {},
      dynamic_config::GetDefaultSource());
  pg::detail::ConnectionPtr conn(nullptr);

  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";
  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained another connection from pool";
  CheckConnection(std::move(conn));

  // We still should have initial count of working connections in the pool
  // NOLINTNEXTLINE(bugprone-use-after-move)
  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool again";
  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained another connection from pool again";
  pg::detail::ConnectionPtr conn2(nullptr);
  UASSERT_NO_THROW(conn2 = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool one more time";
  pool.reset();
  CheckConnection(std::move(conn));
  CheckConnection(std::move(conn2));
}

UTEST_P(PostgrePool, MinPool) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "", GetParam(), {1, 1, 10},
      kCachePreparedStatements, {}, GetTestCmdCtls(),
      testsuite::PostgresControl{}, error_injection::Settings{}, {},
      dynamic_config::GetDefaultSource());
  const auto& stats = pool->GetStatistics();
  EXPECT_EQ(GetParam() == pg::InitMode::kAsync ? 0 : 1,
            stats.connection.open_total);
  EXPECT_EQ(1, stats.connection.active);
  EXPECT_EQ(0, stats.connection.error_total);
}

UTEST_P(PostgrePool, ConnectionCleanup) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "", GetParam(), {1, 1, 10},
      kCachePreparedStatements, {},
      storages::postgres::DefaultCommandControls(
          pg::CommandControl{std::chrono::milliseconds{100},
                             std::chrono::seconds{1}},
          {}, {}),
      testsuite::PostgresControl{}, error_injection::Settings{}, {},
      dynamic_config::GetDefaultSource());

  {
    const auto& stats = pool->GetStatistics();
    EXPECT_EQ(GetParam() == pg::InitMode::kAsync ? 0 : 1,
              stats.connection.open_total);
    EXPECT_EQ(1, stats.connection.active);
    EXPECT_EQ(0, stats.connection.error_total);
  }
  {
    pg::Transaction trx{pg::detail::ConnectionPtr(nullptr)};
    UEXPECT_NO_THROW(trx = pool->Begin({})) << "Start transaction in a pool";

    const auto& stats = pool->GetStatistics();
    EXPECT_EQ(1, stats.connection.open_total);
    EXPECT_EQ(1, stats.connection.active);
    EXPECT_EQ(1, stats.connection.used);
    UEXPECT_THROW(trx.Execute("select pg_sleep(1)"), pg::ConnectionTimeoutError)
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
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "", GetParam(), {1, 1, 10},
      kCachePreparedStatements, {},
      storages::postgres::DefaultCommandControls(
          pg::CommandControl{std::chrono::milliseconds{100},
                             std::chrono::milliseconds{10}},
          {}, {}),
      testsuite::PostgresControl{}, error_injection::Settings{}, {},
      dynamic_config::GetDefaultSource());
  {
    pg::Transaction trx{pg::detail::ConnectionPtr(nullptr)};
    UEXPECT_NO_THROW(trx = pool->Begin({})) << "Start transaction in a pool";

    UEXPECT_THROW(trx.Execute("select pg_sleep(1)"), pg::QueryCancelled)
        << "Fail statement on timeout";
    UEXPECT_NO_THROW(trx.Commit()) << "Connection is left in a usable state";
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

UTEST_P(PostgrePool, SetConnectionSettings) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "", GetParam(), {1, 1, 10},
      kCachePreparedStatements, {}, GetTestCmdCtls(), {}, {}, {},
      dynamic_config::GetDefaultSource());
  pg::detail::ConnectionPtr conn(nullptr);

  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";
  const auto old_settings_version = conn->GetSettings().version;
  conn = pg::detail::ConnectionPtr{nullptr};

  // force pool to recreate connection by assigning new settings
  auto new_settings = kCachePreparedStatements;
  ++new_settings.max_prepared_cache_size;
  pool->SetConnectionSettings(new_settings);

  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";
  const auto new_settings_version = conn->GetSettings().version;
  EXPECT_EQ(new_settings_version, old_settings_version + 1);

  CheckConnection(std::move(conn));
}

UTEST_P(PostgrePool, DefaultCmdCtl) {
  using Source = pg::detail::DefaultCommandControlSource;
  const pg::CommandControl custom_cmd_ctl{std::chrono::seconds{2},
                                          std::chrono::seconds{1}};

  auto default_cmd_ctls = pg::DefaultCommandControls(kTestCmdCtl, {}, {});

  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "", GetParam(), {1, 1, 10},
      kCachePreparedStatements, {}, default_cmd_ctls, {}, {}, {},
      dynamic_config::GetDefaultSource());

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

INSTANTIATE_UTEST_SUITE_P(
    PoolTests, PostgrePool,
    ::testing::Values(pg::InitMode::kAsync, pg::InitMode::kSync),
    [](const testing::TestParamInfo<PostgrePool::ParamType>& info) {
      if (info.param == pg::InitMode::kAsync) {
        return "Async";
      } else {
        return "Sync";
      }
    });

USERVER_NAMESPACE_END
