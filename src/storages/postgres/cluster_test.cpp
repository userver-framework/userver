#include <storages/postgres/util_test.hpp>

#include <gtest/gtest.h>

#include <utest/utest.hpp>

#include <storages/postgres/cluster.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>

#include <storages/postgres/detail/cluster_impl.hpp>

namespace pg = storages::postgres;

namespace {

void CheckTransaction(pg::Transaction trx) {
  pg::ResultSet res{nullptr};

  // TODO Check idle connection count before and after begin
  EXPECT_NO_THROW(res = trx.Execute("select 1"));
  EXPECT_TRUE((bool)res) << "Result set is obtained";
  // TODO Check idle connection count before and after commit
  EXPECT_NO_THROW(trx.Commit());
  EXPECT_THROW(trx.Commit(), pg::NotInTransaction);
  EXPECT_THROW(trx.Rollback(), pg::NotInTransaction);
}

pg::Cluster CreateCluster(const pg::DSNList& dsn_list,
                          engine::TaskProcessor& bg_task_processor,
                          size_t max_size) {
  auto cluster_impl = std::make_unique<pg::detail::ClusterImpl>(
      dsn_list, bg_task_processor, 0, max_size);
  return pg::Cluster(std::move(cluster_impl));
}

}  // namespace

class PostgreCluster : public PostgreSQLBase {};

INSTANTIATE_TEST_CASE_P(/*empty*/, PostgreCluster,
                        ::testing::ValuesIn(GetDsnFromEnv()), DsnToString);

TEST_P(PostgreCluster, ClusterSyncSlaveRW) {
  RunInCoro([this] {
    auto cluster = CreateCluster(dsn_list_, GetTaskProcessor(), 1);

    EXPECT_THROW(cluster.Begin(pg::ClusterHostType::kSyncSlave,
                               pg::TransactionOptions{
                                   pg::TransactionOptions::Mode::kReadWrite}),
                 pg::ClusterUnavailable);
  });
}

TEST_P(PostgreCluster, ClusterAsyncSlaveRW) {
  RunInCoro([this] {
    auto cluster = CreateCluster(dsn_list_, GetTaskProcessor(), 1);

    EXPECT_THROW(cluster.Begin(pg::ClusterHostType::kSlave,
                               pg::TransactionOptions{
                                   pg::TransactionOptions::Mode::kReadWrite}),
                 pg::ClusterUnavailable);
  });
}

TEST_P(PostgreCluster, ClusterEmptyPool) {
  RunInCoro([this] {
    auto cluster = CreateCluster(dsn_list_, GetTaskProcessor(), 0);

    EXPECT_THROW(cluster.Begin({}), pg::PoolError);
  });
}

TEST_P(PostgreCluster, ClusterTransaction) {
  RunInCoro([this] {
    auto cluster = CreateCluster(dsn_list_, GetTaskProcessor(), 1);

    CheckTransaction(cluster.Begin({}));
  });
}
