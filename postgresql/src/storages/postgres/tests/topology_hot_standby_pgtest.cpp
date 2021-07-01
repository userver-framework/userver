#include <gtest/gtest.h>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/topology/hot_standby.hpp>
#include <storages/postgres/exceptions.hpp>

#include <storages/postgres/tests/util_pgtest.hpp>

namespace pg = storages::postgres;

class HotStandby : public PostgreSQLBase,
                   public ::testing::WithParamInterface<pg::DsnList> {};

UTEST_P(HotStandby, Smoke) {
  const auto& dsns = GetParam();
  if (dsns.empty()) return;

  pg::detail::topology::HotStandby qcc(
      GetTaskProcessor(), dsns, pg::TopologySettings{kMaxTestWaitTime},
      pg::ConnectionSettings{}, GetTestCmdCtls(), testsuite::PostgresControl{},
      error_injection::Settings{});
  auto hosts = qcc.GetDsnIndicesByType();

  EXPECT_EQ(1, hosts->count(pg::ClusterHostType::kMaster));
  if (dsns.size() > 1) {
    // Should detect slaves
    EXPECT_EQ(1, hosts->count(pg::ClusterHostType::kSlave));
    EXPECT_LT(0, hosts->at(pg::ClusterHostType::kSlave).size());
  } else {
    EXPECT_EQ(0, hosts->count(pg::ClusterHostType::kSlave));
  }
}

UTEST_P(HotStandby, ReplicationLag) {
  const auto& dsns = GetParam();
  if (dsns.empty()) return;

  pg::detail::topology::HotStandby qcc(
      GetTaskProcessor(), dsns, pg::TopologySettings{std::chrono::seconds{-1}},
      pg::ConnectionSettings{}, GetTestCmdCtls(), testsuite::PostgresControl{},
      error_injection::Settings{});
  auto hosts = qcc.GetDsnIndicesByType();

  EXPECT_EQ(1, hosts->count(pg::ClusterHostType::kMaster));
  // Slaves should be excluded due to unsatisfied lag requirements
  EXPECT_EQ(0, hosts->count(pg::ClusterHostType::kSlave));
}

INSTANTIATE_UTEST_SUITE_P(PostgreTopology, HotStandby,
                          ::testing::ValuesIn(GetDsnListsFromEnv()),
                          DsnListToString);
