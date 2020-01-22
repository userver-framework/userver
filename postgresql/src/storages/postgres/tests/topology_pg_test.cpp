#include <gtest/gtest.h>

#include <engine/standalone.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/quorum_commit.hpp>
#include <storages/postgres/exceptions.hpp>

#include <storages/postgres/tests/util_test.hpp>

namespace pg = storages::postgres;

std::vector<std::string> GetClusterFromEnv() {
  const auto* cluster_dsn = std::getenv("POSTGRES_CLUSTER_TEST");
  return cluster_dsn ? std::vector<std::string>{{cluster_dsn}}
                     : std::vector<std::string>{};
}

class PostgreTopology : public PostgreSQLBase,
                        public ::testing::WithParamInterface<std::string> {
  void ReadParam() override { conn_string_ = GetParam(); };

 protected:
  std::string conn_string_;
};

TEST_P(PostgreTopology, RunTest) {
  auto dsns = pg::SplitByHost(conn_string_);
  ASSERT_FALSE(dsns.empty());

  if (dsns.size() < 2) {
    LOG_WARNING() << "Running topology test with a single host is useless";
  }
  RunInCoro([&dsns] {
    error_injection::Settings ei_settings;
    pg::detail::QuorumCommitCluster qcc(GetTaskProcessor(), dsns,
                                        pg::ConnectionSettings{}, kTestCmdCtl,
                                        ei_settings);
    auto hosts = qcc.GetHostsByType();

    EXPECT_EQ(1, hosts.count(pg::ClusterHostType::kMaster));
    if (dsns.size() > 1) {
      EXPECT_LT(0, hosts.count(pg::ClusterHostType::kSyncSlave));
      // Should detect slaves
      EXPECT_EQ(1, hosts.count(pg::ClusterHostType::kSlave));
      EXPECT_LT(0, hosts.at(pg::ClusterHostType::kSlave).size());
    }
  });
}

INSTANTIATE_TEST_CASE_P(/*empty*/, PostgreTopology,
                        ::testing::ValuesIn(GetClusterFromEnv()), DsnToString);
