#include <userver/utest/utest.hpp>

#include <userver/storages/odbc.hpp>
#include <userver/storages/odbc/tests/utils.hpp>

#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/testing.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

UTEST(OdbcMetrics, ConnectionsBasic) {
    storages::odbc::Cluster cluster(kSettings);

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    UEXPECT_NO_THROW(cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1"));

    const utils::statistics::Snapshot snapshot{
        statistics_storage,
        "odbc.connections",
        {{"odbc_pool", "0"}},
    };

    EXPECT_GE(snapshot.SingleMetric("created").AsRate(), 1);
    EXPECT_EQ(snapshot.SingleMetric("closed").AsRate(), 0);
    EXPECT_GE(snapshot.SingleMetric("active").AsInt(), 1);
    EXPECT_EQ(snapshot.SingleMetric("overload").AsRate(), 0);

    entry.Unregister();
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END

