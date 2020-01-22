#include <storages/postgres/tests/util_test.hpp>

#include <gtest/gtest.h>

#include <utest/utest.hpp>

#include <storages/postgres/cluster.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>

namespace pg = storages::postgres;

namespace {

void CheckTransaction(pg::Transaction trx) {
  pg::ResultSet res{nullptr};

  // TODO Check idle connection count before and after begin
  EXPECT_NO_THROW(res = trx.Execute("select 1"));
  EXPECT_FALSE(res.IsEmpty()) << "Result set is obtained";
  // TODO Check idle connection count before and after commit
  EXPECT_NO_THROW(trx.Commit());
  EXPECT_THROW(trx.Commit(), pg::NotInTransaction);
  EXPECT_NO_THROW(trx.Rollback());
}

pg::Cluster CreateCluster(
    const std::string& dsn, engine::TaskProcessor& bg_task_processor,
    size_t max_size,
    pg::ConnectionSettings conn_settings = kCachePreparedStatements) {
  return pg::Cluster({dsn}, bg_task_processor, {0, max_size, max_size},
                     conn_settings, kTestCmdCtl, {});
}

}  // namespace

class PostgreCluster : public PostgreSQLBase,
                       public ::testing::WithParamInterface<std::string> {
  void ReadParam() override { dsn_ = GetParam(); }

 protected:
  std::string dsn_;
};

INSTANTIATE_TEST_CASE_P(
    /*empty*/, PostgreCluster, ::testing::ValuesIn(GetDsnFromEnv()),
    DsnToString);

TEST_P(PostgreCluster, ClusterSyncSlaveRW) {
  RunInCoro([this] {
    auto cluster = CreateCluster(dsn_, GetTaskProcessor(), 1);

    EXPECT_THROW(
        cluster.Begin(pg::ClusterHostType::kSyncSlave, pg::Transaction::RW),
        pg::ClusterUnavailable);
  });
}

TEST_P(PostgreCluster, ClusterAsyncSlaveRW) {
  RunInCoro([this] {
    auto cluster = CreateCluster(dsn_, GetTaskProcessor(), 1);

    EXPECT_THROW(
        cluster.Begin(pg::ClusterHostType::kSlave, pg::Transaction::RW),
        pg::ClusterUnavailable);
  });
}

TEST_P(PostgreCluster, ClusterEmptyPool) {
  RunInCoro([this] {
    auto cluster = CreateCluster(dsn_, GetTaskProcessor(), 0);

    EXPECT_THROW(cluster.Begin({}), pg::PoolError);
  });
}

TEST_P(PostgreCluster, ClusterTransaction) {
  RunInCoro([this] {
    auto cluster = CreateCluster(dsn_, GetTaskProcessor(), 1);

    CheckTransaction(cluster.Begin({}));
  });
}

TEST_P(PostgreCluster, SingleQuery) {
  RunInCoro([this] {
    auto cluster = CreateCluster(dsn_, GetTaskProcessor(), 1);

    EXPECT_THROW(cluster.Execute(pg::ClusterHostType::kAny, "select 1"),
                 pg::LogicError);
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
  });
}

TEST_P(PostgreCluster, TransactionTimouts) {
  RunInCoro([this] {
    auto cluster = CreateCluster(dsn_, GetTaskProcessor(), 1);

    {
      // Default transaction timeout
      auto trx = cluster.Begin(pg::Transaction::RW);
      EXPECT_ANY_THROW(trx.Execute("select pg_sleep(0.2)"));
      trx.Rollback();
    }
    {
      // Custom transaction timeout
      auto trx = cluster.Begin(pg::Transaction::RW,
                               pg::CommandControl{pg::TimeoutDuration{1000},
                                                  pg::TimeoutDuration{500}});
      EXPECT_NO_THROW(trx.Execute("select pg_sleep(0.2)"));
      trx.Commit();
    }
  });
}
