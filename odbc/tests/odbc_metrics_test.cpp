#include <userver/utest/utest.hpp>

#include <userver/storages/odbc.hpp>
#include <userver/storages/odbc/tests/utils.hpp>

#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/testing.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

UTEST(OdbcMetrics, ConnectionsBasic) {
    storages::odbc::Cluster cluster(kSettings, nullptr);

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    UEXPECT_NO_THROW(cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1"));

    const utils::statistics::Snapshot snapshot{
        statistics_storage,
        "odbc",
        {{"odbc_pool", "0"}},
    };

    // Connection metrics
    EXPECT_GE(snapshot.SingleMetric("connections.opened").AsRate(), 1);
    EXPECT_EQ(snapshot.SingleMetric("connections.closed").AsRate(), 0);
    EXPECT_GE(snapshot.SingleMetric("connections.active").AsInt(), 1);

    // Query metrics
    EXPECT_GE(snapshot.SingleMetric("queries.executed").AsRate(), 1);

    // Transaction metrics (out-of-transaction query)
    EXPECT_GE(snapshot.SingleMetric("transactions.no-tran").AsRate(), 1);

    entry.Unregister();
}

UTEST(OdbcMetrics, TransactionMetrics) {
    storages::odbc::Cluster cluster(kSettings, nullptr);

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    {
        auto tx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
        UEXPECT_NO_THROW(tx.Execute("SELECT 1"));
        UEXPECT_NO_THROW(tx.Execute("SELECT 2"));
        tx.Commit();
    }

    const utils::statistics::Snapshot snapshot{
        statistics_storage,
        "odbc",
        {{"odbc_pool", "0"}},
    };

    // Transaction metrics
    EXPECT_GE(snapshot.SingleMetric("transactions.total").AsRate(), 1);
    EXPECT_GE(snapshot.SingleMetric("transactions.committed").AsRate(), 1);
    EXPECT_EQ(snapshot.SingleMetric("transactions.rolled-back").AsRate(), 0);

    // Query metrics (2 queries in transaction)
    EXPECT_GE(snapshot.SingleMetric("queries.executed").AsRate(), 2);

    entry.Unregister();
}

UTEST(OdbcMetrics, RollbackMetrics) {
    storages::odbc::Cluster cluster(kSettings, nullptr);

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    {
        auto tx = cluster.Begin(storages::odbc::ClusterHostType::kMaster);
        UEXPECT_NO_THROW(tx.Execute("SELECT 1"));
        tx.Rollback();
    }

    const utils::statistics::Snapshot snapshot{
        statistics_storage,
        "odbc",
        {{"odbc_pool", "0"}},
    };

    EXPECT_GE(snapshot.SingleMetric("transactions.total").AsRate(), 1);
    EXPECT_EQ(snapshot.SingleMetric("transactions.committed").AsRate(), 0);
    EXPECT_GE(snapshot.SingleMetric("transactions.rolled-back").AsRate(), 1);

    entry.Unregister();
}

UTEST(OdbcMetrics, ErrorMetrics) {
    storages::odbc::Cluster cluster(kSettings, nullptr);

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    // Execute a valid query first to ensure connection is established
    UEXPECT_NO_THROW(cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1"));

    // Try to execute an invalid query
    UEXPECT_THROW(
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "INVALID SQL SYNTAX"),
        storages::odbc::Error
    );

    const utils::statistics::Snapshot snapshot{
        statistics_storage,
        "odbc",
        {{"odbc_pool", "0"}},
    };

    EXPECT_GE(snapshot.SingleMetric("errors", {{"odbc_error", "query-exec"}}).AsRate(), 1);

    entry.Unregister();
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
