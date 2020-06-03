#include <storages/postgres/tests/util_pgtest.hpp>

#include <gtest/gtest.h>

#include <utest/utest.hpp>

#include <storages/postgres/cluster.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>

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
  return pg::Cluster({dsn}, bg_task_processor, {0, max_size, max_size},
                     conn_settings, kTestCmdCtl, {}, {});
}

}  // namespace

class PostgreCluster : public PostgreSQLBase,
                       public ::testing::WithParamInterface<pg::Dsn> {};

INSTANTIATE_TEST_SUITE_P(/*empty*/, PostgreCluster,
                         ::testing::ValuesIn(GetDsnFromEnv()), DsnToString);

TEST_P(PostgreCluster, ClusterEmptyPool) {
  RunInCoro([] {
    auto cluster = CreateCluster(GetParam(), GetTaskProcessor(), 0);

    EXPECT_THROW(cluster.Begin({}), pg::PoolError);
  });
}

TEST_P(PostgreCluster, ClusterSlaveRW) {
  RunInCoro([] {
    auto cluster = CreateCluster(GetParam(), GetTaskProcessor(), 1);

    EXPECT_THROW(cluster.Begin(pg::ClusterHostType::kSlave, {}),
                 pg::ClusterUnavailable);
    EXPECT_THROW(
        cluster.Begin(pg::ClusterHostType::kSlave, pg::Transaction::RW),
        pg::ClusterUnavailable);
    EXPECT_THROW(cluster.Begin({pg::ClusterHostType::kSlave,
                                pg::ClusterHostType::kRoundRobin},
                               {}),
                 pg::ClusterUnavailable);
    EXPECT_THROW(cluster.Begin({pg::ClusterHostType::kSlave,
                                pg::ClusterHostType::kRoundRobin},
                               pg::Transaction::RW),
                 pg::ClusterUnavailable);
    EXPECT_THROW(
        cluster.Begin(
            {pg::ClusterHostType::kSlave, pg::ClusterHostType::kNearest}, {}),
        pg::ClusterUnavailable);
    EXPECT_THROW(cluster.Begin({pg::ClusterHostType::kSlave,
                                pg::ClusterHostType::kNearest},
                               pg::Transaction::RW),
                 pg::ClusterUnavailable);
  });
}

TEST_P(PostgreCluster, ClusterSyncSlaveRW) {
  RunInCoro([] {
    auto cluster = CreateCluster(GetParam(), GetTaskProcessor(), 1);

    EXPECT_THROW(cluster.Begin(pg::ClusterHostType::kSyncSlave, {}),
                 pg::ClusterUnavailable);
    EXPECT_THROW(
        cluster.Begin(pg::ClusterHostType::kSyncSlave, pg::Transaction::RW),
        pg::ClusterUnavailable);
  });
}

TEST_P(PostgreCluster, ClusterMasterRW) {
  RunInCoro([] {
    auto cluster = CreateCluster(GetParam(), GetTaskProcessor(), 1);

    CheckRwTransaction(cluster.Begin({}));
    CheckRwTransaction(cluster.Begin(pg::Transaction::RW));
    CheckRwTransaction(cluster.Begin(pg::ClusterHostType::kMaster, {}));
    CheckRwTransaction(
        cluster.Begin(pg::ClusterHostType::kMaster, pg::Transaction::RW));
  });
}

TEST_P(PostgreCluster, ClusterAnyRW) {
  RunInCoro([] {
    auto cluster = CreateCluster(GetParam(), GetTaskProcessor(), 1);

    CheckRwTransaction(cluster.Begin(
        {pg::ClusterHostType::kMaster, pg::ClusterHostType::kSlave}, {}));
    CheckRwTransaction(cluster.Begin(
        {pg::ClusterHostType::kMaster, pg::ClusterHostType::kSlave},
        pg::Transaction::RW));
    CheckRwTransaction(cluster.Begin(
        {pg::ClusterHostType::kMaster, pg::ClusterHostType::kSlave,
         pg::ClusterHostType::kRoundRobin},
        {}));
    CheckRwTransaction(cluster.Begin(
        {pg::ClusterHostType::kMaster, pg::ClusterHostType::kSlave,
         pg::ClusterHostType::kRoundRobin},
        pg::Transaction::RW));
    CheckRwTransaction(cluster.Begin(
        {pg::ClusterHostType::kMaster, pg::ClusterHostType::kSlave,
         pg::ClusterHostType::kNearest},
        {}));
    CheckRwTransaction(cluster.Begin(
        {pg::ClusterHostType::kMaster, pg::ClusterHostType::kSlave,
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
  });
}

TEST_P(PostgreCluster, ClusterSlaveRO) {
  RunInCoro([] {
    auto cluster = CreateCluster(GetParam(), GetTaskProcessor(), 1);

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
  });
}

TEST_P(PostgreCluster, ClusterSyncSlaveRO) {
  RunInCoro([] {
    auto cluster = CreateCluster(GetParam(), GetTaskProcessor(), 1);

    CheckRoTransaction(
        cluster.Begin(pg::ClusterHostType::kSyncSlave, pg::Transaction::RO));
  });
}

TEST_P(PostgreCluster, ClusterMasterRO) {
  RunInCoro([] {
    auto cluster = CreateCluster(GetParam(), GetTaskProcessor(), 1);

    CheckRoTransaction(
        cluster.Begin(pg::ClusterHostType::kMaster, pg::Transaction::RO));
  });
}

TEST_P(PostgreCluster, ClusterAnyRO) {
  RunInCoro([] {
    auto cluster = CreateCluster(GetParam(), GetTaskProcessor(), 1);

    CheckRoTransaction(cluster.Begin(
        {pg::ClusterHostType::kSlave, pg::ClusterHostType::kMaster},
        pg::Transaction::RO));
    CheckRoTransaction(cluster.Begin(
        {pg::ClusterHostType::kSlave, pg::ClusterHostType::kMaster,
         pg::ClusterHostType::kRoundRobin},
        pg::Transaction::RO));
    CheckRoTransaction(cluster.Begin(
        {pg::ClusterHostType::kSlave, pg::ClusterHostType::kMaster,
         pg::ClusterHostType::kNearest},
        pg::Transaction::RO));

    EXPECT_THROW(
        cluster.Begin(
            {pg::ClusterHostType::kSlave, pg::ClusterHostType::kMaster,
             pg::ClusterHostType::kRoundRobin, pg::ClusterHostType::kNearest},
            pg::Transaction::RO),
        pg::LogicError);
  });
}

TEST_P(PostgreCluster, SingleQuery) {
  RunInCoro([] {
    auto cluster = CreateCluster(GetParam(), GetTaskProcessor(), 1);

    EXPECT_THROW(cluster.Execute({}, "select 1"), pg::LogicError);

    pg::ResultSet res{nullptr};
    EXPECT_NO_THROW(
        res = cluster.Execute(pg::ClusterHostType::kMaster, "select 1"));
    EXPECT_EQ(1, res.Size());
    EXPECT_NO_THROW(
        res = cluster.Execute(pg::ClusterHostType::kSyncSlave, "select 1"));
    EXPECT_EQ(1, res.Size());
    EXPECT_NO_THROW(
        res = cluster.Execute(pg::ClusterHostType::kSlave, "select 1"));
    EXPECT_EQ(1, res.Size());
    EXPECT_NO_THROW(res = cluster.Execute({pg::ClusterHostType::kMaster,
                                           pg::ClusterHostType::kSlave},
                                          "select 1"));
    EXPECT_EQ(1, res.Size());
  });
}

TEST_P(PostgreCluster, HostSelectionSingleQuery) {
  RunInCoro([] {
    auto cluster = CreateCluster(GetParam(), GetTaskProcessor(), 1);

    EXPECT_THROW(
        cluster.Execute({pg::ClusterHostType::kRoundRobin}, "select 1"),
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
  });
}

TEST_P(PostgreCluster, TransactionTimeouts) {
  RunInCoro([] {
    auto cluster = CreateCluster(GetParam(), GetTaskProcessor(), 1);

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
  });
}
