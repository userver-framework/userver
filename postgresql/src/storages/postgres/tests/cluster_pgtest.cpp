#include <storages/postgres/tests/util_pgtest.hpp>

#include <gtest/gtest.h>

#include <userver/utest/utest.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

namespace {

enum class CheckTxnType { kRw, kRo };

void CheckTransaction(pg::Transaction trx, CheckTxnType txn_type) {
  pg::ResultSet res{nullptr};

  // TODO Check idle connection count before and after begin
  EXPECT_NO_THROW(res = trx.Execute(
                      "SELECT current_setting('transaction_read_only')::bool"));
  EXPECT_FALSE(res.IsEmpty()) << "Result set is obtained";
  EXPECT_EQ(txn_type == CheckTxnType::kRo, res.AsSingleRow<bool>());
  // TODO Check idle connection count before and after commit
  EXPECT_NO_THROW(trx.Commit());
  EXPECT_THROW(trx.Commit(), pg::NotInTransaction);
  EXPECT_NO_THROW(trx.Rollback());
}

void CheckRoTransaction(pg::Transaction trx) {
  CheckTransaction(std::move(trx), CheckTxnType::kRo);
}

void CheckRwTransaction(pg::Transaction trx) {
  CheckTransaction(std::move(trx), CheckTxnType::kRw);
}

pg::Cluster CreateCluster(
    const pg::Dsn& dsn, engine::TaskProcessor& bg_task_processor,
    size_t max_size,
    pg::ConnectionSettings conn_settings = kCachePreparedStatements) {
  return pg::Cluster({dsn}, nullptr, bg_task_processor,
                     {{},
                      {},
                      {kMaxTestWaitTime},
                      {0, max_size, max_size},
                      conn_settings,
                      storages::postgres::InitMode::kAsync,
                      ""},
                     {kTestCmdCtl, {}, {}}, {}, {});
}

}  // namespace

class PostgreCluster : public PostgreSQLBase {};

UTEST_F(PostgreCluster, ClusterEmptyPool) {
  auto cluster = CreateCluster(GetDsnFromEnv(), GetTaskProcessor(), 0);

  EXPECT_THROW(cluster.Begin({}), pg::PoolError);
}

UTEST_F(PostgreCluster, ClusterSlaveRW) {
  auto cluster = CreateCluster(GetDsnFromEnv(), GetTaskProcessor(), 1);

  EXPECT_THROW(cluster.Begin(pg::ClusterHostType::kSlave, {}),
               pg::ClusterUnavailable);
  EXPECT_THROW(cluster.Begin(pg::ClusterHostType::kSlave, pg::Transaction::RW),
               pg::ClusterUnavailable);
  EXPECT_THROW(
      cluster.Begin(
          {pg::ClusterHostType::kSlave, pg::ClusterHostType::kRoundRobin}, {}),
      pg::ClusterUnavailable);
  EXPECT_THROW(cluster.Begin({pg::ClusterHostType::kSlave,
                              pg::ClusterHostType::kRoundRobin},
                             pg::Transaction::RW),
               pg::ClusterUnavailable);
  EXPECT_THROW(
      cluster.Begin(
          {pg::ClusterHostType::kSlave, pg::ClusterHostType::kNearest}, {}),
      pg::ClusterUnavailable);
  EXPECT_THROW(cluster.Begin(
                   {pg::ClusterHostType::kSlave, pg::ClusterHostType::kNearest},
                   pg::Transaction::RW),
               pg::ClusterUnavailable);
}

UTEST_F(PostgreCluster, ClusterSyncSlaveRW) {
  auto cluster = CreateCluster(GetDsnFromEnv(), GetTaskProcessor(), 1);

  EXPECT_THROW(cluster.Begin(pg::ClusterHostType::kSyncSlave, {}),
               pg::ClusterUnavailable);
  EXPECT_THROW(
      cluster.Begin(pg::ClusterHostType::kSyncSlave, pg::Transaction::RW),
      pg::ClusterUnavailable);
}

UTEST_F(PostgreCluster, ClusterMasterRW) {
  auto cluster = CreateCluster(GetDsnFromEnv(), GetTaskProcessor(), 1);

  CheckRwTransaction(cluster.Begin({}));
  CheckRwTransaction(cluster.Begin(pg::Transaction::RW));
  CheckRwTransaction(cluster.Begin(pg::ClusterHostType::kMaster, {}));
  CheckRwTransaction(
      cluster.Begin(pg::ClusterHostType::kMaster, pg::Transaction::RW));
}

UTEST_F(PostgreCluster, ClusterAnyRW) {
  auto cluster = CreateCluster(GetDsnFromEnv(), GetTaskProcessor(), 1);

  CheckRwTransaction(cluster.Begin(
      {pg::ClusterHostType::kMaster, pg::ClusterHostType::kSlave}, {}));
  CheckRwTransaction(
      cluster.Begin({pg::ClusterHostType::kMaster, pg::ClusterHostType::kSlave},
                    pg::Transaction::RW));
  CheckRwTransaction(
      cluster.Begin({pg::ClusterHostType::kMaster, pg::ClusterHostType::kSlave,
                     pg::ClusterHostType::kRoundRobin},
                    {}));
  CheckRwTransaction(
      cluster.Begin({pg::ClusterHostType::kMaster, pg::ClusterHostType::kSlave,
                     pg::ClusterHostType::kRoundRobin},
                    pg::Transaction::RW));
  CheckRwTransaction(
      cluster.Begin({pg::ClusterHostType::kMaster, pg::ClusterHostType::kSlave,
                     pg::ClusterHostType::kNearest},
                    {}));
  CheckRwTransaction(
      cluster.Begin({pg::ClusterHostType::kMaster, pg::ClusterHostType::kSlave,
                     pg::ClusterHostType::kNearest},
                    pg::Transaction::RW));

  EXPECT_THROW(
      cluster.Begin(
          {pg::ClusterHostType::kMaster, pg::ClusterHostType::kSlave,
           pg::ClusterHostType::kRoundRobin, pg::ClusterHostType::kNearest},
          {}),
      pg::LogicError);
  EXPECT_THROW(
      cluster.Begin(
          {pg::ClusterHostType::kMaster, pg::ClusterHostType::kSlave,
           pg::ClusterHostType::kRoundRobin, pg::ClusterHostType::kNearest},
          pg::Transaction::RW),
      pg::LogicError);
}

UTEST_F(PostgreCluster, ClusterSlaveRO) {
  auto cluster = CreateCluster(GetDsnFromEnv(), GetTaskProcessor(), 1);

  CheckRoTransaction(cluster.Begin(pg::Transaction::RO));
  CheckRoTransaction(
      cluster.Begin(pg::ClusterHostType::kSlave, pg::Transaction::RO));
  CheckRoTransaction(cluster.Begin(
      {pg::ClusterHostType::kSlave, pg::ClusterHostType::kRoundRobin},
      pg::Transaction::RO));
  CheckRoTransaction(cluster.Begin(
      {pg::ClusterHostType::kSlave, pg::ClusterHostType::kNearest},
      pg::Transaction::RO));

  EXPECT_THROW(cluster.Begin({pg::ClusterHostType::kSlave,
                              pg::ClusterHostType::kRoundRobin,
                              pg::ClusterHostType::kNearest},
                             pg::Transaction::RO),
               pg::LogicError);
}

UTEST_F(PostgreCluster, ClusterSyncSlaveRO) {
  auto cluster = CreateCluster(GetDsnFromEnv(), GetTaskProcessor(), 1);

  CheckRoTransaction(
      cluster.Begin(pg::ClusterHostType::kSyncSlave, pg::Transaction::RO));
}

UTEST_F(PostgreCluster, ClusterMasterRO) {
  auto cluster = CreateCluster(GetDsnFromEnv(), GetTaskProcessor(), 1);

  CheckRoTransaction(
      cluster.Begin(pg::ClusterHostType::kMaster, pg::Transaction::RO));
}

UTEST_F(PostgreCluster, ClusterAnyRO) {
  auto cluster = CreateCluster(GetDsnFromEnv(), GetTaskProcessor(), 1);

  CheckRoTransaction(
      cluster.Begin({pg::ClusterHostType::kSlave, pg::ClusterHostType::kMaster},
                    pg::Transaction::RO));
  CheckRoTransaction(
      cluster.Begin({pg::ClusterHostType::kSlave, pg::ClusterHostType::kMaster,
                     pg::ClusterHostType::kRoundRobin},
                    pg::Transaction::RO));
  CheckRoTransaction(
      cluster.Begin({pg::ClusterHostType::kSlave, pg::ClusterHostType::kMaster,
                     pg::ClusterHostType::kNearest},
                    pg::Transaction::RO));

  EXPECT_THROW(
      cluster.Begin(
          {pg::ClusterHostType::kSlave, pg::ClusterHostType::kMaster,
           pg::ClusterHostType::kRoundRobin, pg::ClusterHostType::kNearest},
          pg::Transaction::RO),
      pg::LogicError);
}

UTEST_F(PostgreCluster, SingleQuery) {
  auto cluster = CreateCluster(GetDsnFromEnv(), GetTaskProcessor(), 1);

  EXPECT_THROW(cluster.Execute({}, "select 1"), pg::LogicError);

  pg::ResultSet res{nullptr};
  EXPECT_NO_THROW(
      res = cluster.Execute(pg::ClusterHostType::kMaster, "select 1"));
  EXPECT_EQ(1, res.Size());
  EXPECT_NO_THROW(
      res = cluster.Execute(pg::ClusterHostType::kSyncSlave, "select 1"));
  EXPECT_EQ(1, res.Size());
  EXPECT_NO_THROW(res =
                      cluster.Execute(pg::ClusterHostType::kSlave, "select 1"));
  EXPECT_EQ(1, res.Size());
  EXPECT_NO_THROW(res = cluster.Execute({pg::ClusterHostType::kMaster,
                                         pg::ClusterHostType::kSlave},
                                        "select 1"));
  EXPECT_EQ(1, res.Size());
}

UTEST_F(PostgreCluster, HostSelectionSingleQuery) {
  auto cluster = CreateCluster(GetDsnFromEnv(), GetTaskProcessor(), 1);

  EXPECT_THROW(cluster.Execute({pg::ClusterHostType::kRoundRobin}, "select 1"),
               pg::LogicError);
  EXPECT_THROW(cluster.Execute({pg::ClusterHostType::kNearest}, "select 1"),
               pg::LogicError);
  EXPECT_THROW(cluster.Execute({pg::ClusterHostType::kNearest,
                                pg::ClusterHostType::kRoundRobin},
                               "select 1"),
               pg::LogicError);

  pg::ResultSet res{nullptr};
  EXPECT_NO_THROW(res = cluster.Execute({pg::ClusterHostType::kSlave,
                                         pg::ClusterHostType::kRoundRobin},
                                        "select 1"));
  EXPECT_EQ(1, res.Size());
  EXPECT_NO_THROW(res = cluster.Execute({pg::ClusterHostType::kSlave,
                                         pg::ClusterHostType::kNearest},
                                        "select 1"));
  EXPECT_EQ(1, res.Size());
  EXPECT_NO_THROW(res = cluster.Execute({pg::ClusterHostType::kSlave,
                                         pg::ClusterHostType::kMaster,
                                         pg::ClusterHostType::kRoundRobin},
                                        "select 1"));
  EXPECT_EQ(1, res.Size());
  EXPECT_NO_THROW(res = cluster.Execute({pg::ClusterHostType::kSlave,
                                         pg::ClusterHostType::kMaster,
                                         pg::ClusterHostType::kNearest},
                                        "select 1"));
  EXPECT_EQ(1, res.Size());

  EXPECT_THROW(cluster.Execute({pg::ClusterHostType::kSlave,
                                pg::ClusterHostType::kRoundRobin,
                                pg::ClusterHostType::kNearest},
                               "select 1"),
               pg::LogicError);
  EXPECT_THROW(
      cluster.Execute(
          {pg::ClusterHostType::kSlave, pg::ClusterHostType::kMaster,
           pg::ClusterHostType::kRoundRobin, pg::ClusterHostType::kNearest},
          "select 1"),
      pg::LogicError);
}

UTEST_F(PostgreCluster, TransactionTimeouts) {
  auto cluster = CreateCluster(GetDsnFromEnv(), GetTaskProcessor(), 1);

  {
    // Default transaction no timeout
    auto trx = cluster.Begin(pg::Transaction::RW);
    EXPECT_NO_THROW(trx.Execute("select pg_sleep(0.1)"));
    trx.Rollback();
  }
  {
    // Default transaction timeout
    auto trx = cluster.Begin(pg::Transaction::RW);
    EXPECT_THROW(trx.Execute("select pg_sleep(10)"), pg::QueryCancelled);
    trx.Rollback();
  }
  {
    // Custom transaction no timeout
    auto trx = cluster.Begin(
        pg::Transaction::RW,
        kTestCmdCtl.WithStatementTimeout(std::chrono::milliseconds{250}));
    EXPECT_NO_THROW(trx.Execute("select pg_sleep(0.1)"));
    trx.Commit();
  }
  {
    // Custom transaction timeout
    auto trx = cluster.Begin(
        pg::Transaction::RW,
        kTestCmdCtl.WithStatementTimeout(std::chrono::milliseconds{50}));
    EXPECT_THROW(trx.Execute("select pg_sleep(0.1)"), pg::QueryCancelled);
    trx.Commit();
  }
  {
    static const std::string kTestTransactionName = "test-transaction-name";
    pg::CommandControlByQueryMap ccq_map{
        {kTestTransactionName,
         kTestCmdCtl.WithStatementTimeout(std::chrono::milliseconds{50})}};
    cluster.SetQueriesCommandControl(ccq_map);
    // Use timeout for custom transaction name
    auto trx = cluster.Begin(kTestTransactionName, pg::Transaction::RW);
    EXPECT_THROW(trx.Execute("select pg_sleep(0.1)"), pg::QueryCancelled);
    trx.Commit();
  }
}

UTEST_F(PostgreCluster, NonTransactionExecuteWithParameterStore) {
  auto cluster = CreateCluster(GetDsnFromEnv(), GetTaskProcessor(), 1);

  {
    auto res = cluster.Execute(pg::ClusterHostType::kMaster, "select $1",
                               pg::ParameterStore{}.PushBack(1));
    EXPECT_EQ(1, res.Size());
  }
  {
    pg::CommandControl cc{std::chrono::milliseconds{50},
                          std::chrono::milliseconds{300}};
    auto res = cluster.Execute(pg::ClusterHostType::kMaster, cc, "select $1",
                               pg::ParameterStore{}.PushBack(1));
    EXPECT_EQ(1, res.Size());
  }
}

USERVER_NAMESPACE_END
