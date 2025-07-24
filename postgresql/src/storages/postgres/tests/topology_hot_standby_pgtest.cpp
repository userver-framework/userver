#include <gtest/gtest.h>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/topology/hot_standby.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>

#include <storages/postgres/tests/util_pgtest.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

class HotStandby : public PostgreSQLBase {};

UTEST_F(HotStandby, Smoke) {
    const auto& dsns = GetDsnListFromEnv();
    const pg::detail::topology::HotStandby qcc(
        GetTaskProcessor(),
        dsns,
        nullptr,
        pg::TopologySettings{utest::kMaxTestWaitTime},
        pg::ConnectionSettings{},
        GetTestCmdCtls(),
        testsuite::PostgresControl{},
        error_injection::Settings{},
        std::make_shared<utils::statistics::MetricsStorage>()
    );
    auto hosts = qcc.GetDsnIndicesByType();

    EXPECT_EQ(1, hosts->count(pg::ClusterHostType::kMaster));
    if (dsns.size() > 1) {
        // Should detect slaves
        EXPECT_EQ(1, hosts->count(pg::ClusterHostType::kSlave));
        EXPECT_LT(0, hosts->at(pg::ClusterHostType::kSlave).indices.size());
    } else {
        EXPECT_EQ(0, hosts->count(pg::ClusterHostType::kSlave));
    }
}

UTEST_F(HotStandby, ReplicationLag) {
    const pg::detail::topology::HotStandby qcc(
        GetTaskProcessor(),
        GetDsnListFromEnv(),
        nullptr,
        pg::TopologySettings{std::chrono::seconds{-1}},
        pg::ConnectionSettings{},
        GetTestCmdCtls(),
        testsuite::PostgresControl{},
        error_injection::Settings{},
        std::make_shared<utils::statistics::MetricsStorage>()
    );
    auto hosts = qcc.GetDsnIndicesByType();

    EXPECT_EQ(1, hosts->count(pg::ClusterHostType::kMaster));
    // Slaves should be excluded due to unsatisfied lag requirements
    EXPECT_EQ(0, hosts->count(pg::ClusterHostType::kSlave));
}

UTEST_F(HotStandby, SetReplicationLag) {
    pg::detail::topology::HotStandby qcc(
        GetTaskProcessor(),
        GetDsnListFromEnv(),
        nullptr,
        pg::TopologySettings{std::chrono::seconds{0}},
        pg::ConnectionSettings{},
        GetTestCmdCtls(),
        testsuite::PostgresControl{},
        error_injection::Settings{},
        std::make_shared<utils::statistics::MetricsStorage>()
    );
    qcc.SetTopologySettings(pg::TopologySettings{std::chrono::milliseconds{60}});
    EXPECT_TRUE(qcc.GetTopologySettings().max_replication_lag == std::chrono::milliseconds(60));
}

USERVER_NAMESPACE_END
