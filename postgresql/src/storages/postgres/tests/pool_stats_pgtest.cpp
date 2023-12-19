#include <storages/postgres/tests/util_pgtest.hpp>

#include <gtest/gtest.h>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/pool.hpp>
#include <storages/postgres/postgres_config.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

namespace {

class PostgrePoolStats : public PostgreSQLBase {};

UTEST_F(PostgrePoolStats, EmptyPool) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "",
      storages::postgres::InitMode::kAsync, {0, 10, 10},
      kCachePreparedStatements, {}, GetTestCmdCtls(), {}, {}, {},
      dynamic_config::GetDefaultSource());

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
      stats.transaction.total_percentile.GetStatsForPeriod().GetPercentile(100),
      0);
  EXPECT_EQ(
      stats.transaction.busy_percentile.GetStatsForPeriod().GetPercentile(100),
      0);
}

UTEST_F(PostgrePoolStats, MinPoolSize) {
  const auto min_pool_size = 2;
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "",
      storages::postgres::InitMode::kAsync, {min_pool_size, 10, 10},
      kCachePreparedStatements, {}, GetTestCmdCtls(), {}, {}, {},
      dynamic_config::GetDefaultSource());

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
      stats.transaction.total_percentile.GetStatsForPeriod().GetPercentile(100),
      0);
  EXPECT_EQ(
      stats.transaction.busy_percentile.GetStatsForPeriod().GetPercentile(100),
      0);
}

UTEST_F(PostgrePoolStats, RunStatement) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "",
      storages::postgres::InitMode::kAsync, {1, 10, 10},
      kCachePreparedStatements, {10}, GetTestCmdCtls(), {}, {}, {},
      dynamic_config::GetDefaultSource());

  const std::string statement_name = "statement_name";
  const auto query = pg::Query{"select 1", pg::Query::Name{statement_name}};

  pg::detail::ConnectionPtr conn{nullptr};
  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";
  CheckConnection(conn);

  auto ntrx = pg::detail::NonTransaction{std::move(conn)};

  UEXPECT_NO_THROW(ntrx.Execute(query));
  pool->GetStatementTimingsStorage().WaitForExhaustion();
  const auto stats = pool->GetStatementTimingsStorage().GetTimingsPercentiles();
  EXPECT_NE(stats.find(statement_name), stats.end());
}

UTEST_F(PostgrePoolStats, RunSingleTransaction) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "",
      storages::postgres::InitMode::kAsync, {1, 10, 10},
      kCachePreparedStatements, {10}, GetTestCmdCtls(), {}, {}, {},
      dynamic_config::GetDefaultSource());

  const std::string statement_name = "statement_name";
  const auto query = pg::Query{"select 1", pg::Query::Name{statement_name}};

  pg::detail::ConnectionPtr conn{nullptr};
  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";
  CheckConnection(conn);

  auto trx = pool->Begin(pg::TransactionOptions{});
  trx.Execute(query);
  trx.Commit();

  pool->GetStatementTimingsStorage().WaitForExhaustion();
  const auto stats = pool->GetStatementTimingsStorage().GetTimingsPercentiles();
  EXPECT_NE(stats.find(statement_name), stats.end());
}

UTEST_F(PostgrePoolStats, RunTransactions) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "",
      storages::postgres::InitMode::kAsync, {1, 10, 10},
      kCachePreparedStatements, {}, GetTestCmdCtls(), {}, {}, {},
      dynamic_config::GetDefaultSource());

  const auto trx_count = 5;
  const auto exec_count = 10;

  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.reserve(trx_count);
  for (auto i = 0; i < trx_count; ++i) {
    tasks.push_back(engine::AsyncNoSpan([&pool] {
      pg::detail::ConnectionPtr conn(nullptr);

      UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
          << "Obtained connection from pool";
      CheckConnection(conn);

      [[maybe_unused]] const auto old_stats = conn->GetStatsAndReset();
      UASSERT_NO_THROW(conn->Begin(pg::TransactionOptions{},
                                   pg::detail::SteadyClock::now()));
      for (auto i = 0; i < exec_count; ++i) {
        UEXPECT_NO_THROW(conn->Execute("select 1"))
            << "select 1 successfully executed";
      }
      UEXPECT_NO_THROW(conn->Commit());
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

  const auto prepared_stats =
      stats.connection.prepared_statements.GetStatsForPeriod(kDurationMin, true)
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
}

UTEST_F(PostgrePoolStats, ConnUsed) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "",
      storages::postgres::InitMode::kAsync, {1, 10, 10},
      kCachePreparedStatements, {}, GetTestCmdCtls(), {}, {}, {},
      dynamic_config::GetDefaultSource());
  pg::detail::ConnectionPtr conn(nullptr);

  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";

  const auto& stats = pool->GetStatistics();
  EXPECT_EQ(stats.connection.used, 1);
}

UTEST_F(PostgrePoolStats, Portal) {
  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "",
      storages::postgres::InitMode::kAsync, {1, 10, 10},
      kCachePreparedStatements, {}, GetTestCmdCtls(), {}, {}, {},
      dynamic_config::GetDefaultSource());

  {
    pg::detail::ConnectionPtr conn(nullptr);

    UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
        << "Obtained connection from pool";
    CheckConnection(conn);

    UEXPECT_NO_THROW(
        conn->Begin(pg::TransactionOptions{}, pg::detail::SteadyClock::now()));
    pg::detail::Connection::StatementId stmt_id;
    UEXPECT_NO_THROW(stmt_id = conn->PortalBind("select 1", "test", {}, {}));
    pg::ResultSet res{nullptr};
    UEXPECT_NO_THROW(res = conn->PortalExecute(stmt_id, "test", 0, {}));
    EXPECT_EQ(res.Size(), 1);
    UEXPECT_NO_THROW(conn->Commit());
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
}

UTEST_F(PostgrePoolStats, MaxPreparedCacheSize) {
  pg::ConnectionSettings conn_settings;
  conn_settings.max_prepared_cache_size = 5;

  auto pool = pg::detail::ConnectionPool::Create(
      GetDsnFromEnv(), nullptr, GetTaskProcessor(), "",
      storages::postgres::InitMode::kAsync, {1, 10, 10}, conn_settings, {},
      GetTestCmdCtls(), {}, {}, {}, dynamic_config::GetDefaultSource());

  auto conn = pg::detail::ConnectionPtr{nullptr};
  UASSERT_NO_THROW(conn = pool->Acquire(MakeDeadline()))
      << "Obtained connection from pool";
  CheckConnection(conn);

  auto old_stats = conn->GetStatsAndReset();
  EXPECT_LT(old_stats.prepared_statements_current,
            conn_settings.max_prepared_cache_size);

  for (size_t i = 0; i < conn_settings.max_prepared_cache_size + 1; ++i) {
    UEXPECT_NO_THROW(conn->Execute("select " + std::to_string(i)));
  }

  auto new_stats = conn->GetStatsAndReset();
  EXPECT_EQ(new_stats.prepared_statements_current,
            conn_settings.max_prepared_cache_size);
}

}  // namespace

USERVER_NAMESPACE_END
