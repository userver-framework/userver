#include <gtest/gtest.h>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/topology/hot_standby.hpp>
#include <userver/storages/postgres/exceptions.hpp>

#include <storages/postgres/tests/util_pgtest.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

class HotStandby : public PostgreSQLBase {};

UTEST_F(HotStandby, Smoke) {
  const auto& dsns = GetDsnListFromEnv();
  pg::detail::topology::HotStandby qcc(
      GetTaskProcessor(), dsns, nullptr,
      pg::TopologySettings{utest::kMaxTestWaitTime}, pg::ConnectionSettings{},
      GetTestCmdCtls(), testsuite::PostgresControl{},
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

UTEST_F(HotStandby, ReplicationLag) {
  pg::detail::topology::HotStandby qcc(
      GetTaskProcessor(), GetDsnListFromEnv(), nullptr,
      pg::TopologySettings{std::chrono::seconds{-1}}, pg::ConnectionSettings{},
      GetTestCmdCtls(), testsuite::PostgresControl{},
      error_injection::Settings{});
  auto hosts = qcc.GetDsnIndicesByType();

  EXPECT_EQ(1, hosts->count(pg::ClusterHostType::kMaster));
  // Slaves should be excluded due to unsatisfied lag requirements
  EXPECT_EQ(0, hosts->count(pg::ClusterHostType::kSlave));
}

USERVER_NAMESPACE_END
