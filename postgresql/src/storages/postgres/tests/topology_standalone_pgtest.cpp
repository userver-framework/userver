#include <gtest/gtest.h>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/topology/standalone.hpp>
#include <userver/error_injection/settings.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>

#include <storages/postgres/tests/util_pgtest.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

class Standalone : public PostgreSQLBase {};

UTEST_F(Standalone, Smoke) {
    const pg::detail::topology::Standalone sa(
        GetTaskProcessor(),
        GetDsnListFromEnv(),
        nullptr,
        pg::TopologySettings{utest::kMaxTestWaitTime},
        pg::ConnectionSettings{},
        GetTestCmdCtls(),
        testsuite::PostgresControl{},
        error_injection::Settings{},
        std::make_shared<utils::statistics::MetricsStorage>()
    );

    auto hosts = sa.GetDsnIndicesByType();
    EXPECT_EQ(1, hosts->count(pg::ClusterHostType::kMaster));
    EXPECT_EQ(0, hosts->count(pg::ClusterHostType::kSlave));

    auto alive = sa.GetAliveDsnIndices();
    EXPECT_EQ(1, alive->indices.size());
}

USERVER_NAMESPACE_END
