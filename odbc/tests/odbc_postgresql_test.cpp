#include <gtest/gtest.h>
#include <userver/storages/odbc.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

constexpr auto kDSN =
    "DRIVER={PostgreSQL Unicode};"
    "SERVER=localhost;"
    "PORT=15433;"
    "DATABASE=postgres;"
    "UID=testsuite;"
    "PWD=password;";

namespace {
auto kHostSettings = storages::odbc::settings::HostSettings{kDSN, {}};
auto kSettings = storages::odbc::settings::ODBCClusterSettings{{kHostSettings}};
}  // namespace

UTEST(CreateConnection, Works) { storages::odbc::Cluster cluster(kSettings); }

UTEST(CreateConnection, MultipleDSN) {
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{kHostSettings, kHostSettings}});
}

UTEST(Query, Works) {
    storages::odbc::Cluster cluster(kSettings);

    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
    EXPECT_EQ(result.Size(), 1);
    EXPECT_FALSE(result.IsEmpty());
    auto row = result[0];
    EXPECT_EQ(row.Size(), 1);
    EXPECT_FALSE(row[0].IsNull());
    EXPECT_EQ(row[0].GetInt32(), 1);

    auto multipleRows = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT generate_series(1, 10)");
    EXPECT_EQ(multipleRows.Size(), 10);
    for (std::size_t i = 0; i < multipleRows.Size(); i++) {
        auto row = multipleRows[i];
        EXPECT_EQ(row.Size(), 1);
        EXPECT_FALSE(row[0].IsNull());
        EXPECT_EQ(row[0].GetInt32(), i + 1);
    }
}

UTEST(Query, VariousTypes) {
    auto query = "SELECT 42, 'test', 1.0, false, null, true";
    storages::odbc::Cluster cluster(kSettings);

    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, query);
    EXPECT_EQ(result.Size(), 1);
    EXPECT_FALSE(result.IsEmpty());
    auto row = result[0];
    EXPECT_EQ(row.Size(), 6);

    auto intField = row[0];
    EXPECT_EQ(intField.GetInt32(), 42);

    auto stringField = row[1];
    EXPECT_EQ(stringField.GetString(), "test");

    auto doubleField = row[2];
    EXPECT_DOUBLE_EQ(doubleField.GetDouble(), 1.0);

    auto boolField = row[3];
    EXPECT_EQ(boolField.GetBool(), false);

    auto nullField = row[4];
    EXPECT_TRUE(nullField.IsNull());

    auto trueBool = row[5];
    EXPECT_EQ(trueBool.GetBool(), true);
}

UTEST(Query, DifferentHostTypes) {
    auto query = "SELECT 1";
    storages::odbc::Cluster cluster(kSettings);

    // TODO: needs an actual check that host are selected correctly
    cluster.Execute(storages::odbc::ClusterHostType::kMaster, query);
    cluster.Execute(storages::odbc::ClusterHostType::kSlave, query);
    cluster.Execute(storages::odbc::ClusterHostType::kNone, query);
}

UTEST(Pool, LessQueriesThanConnections) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}});

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections);

    for (std::size_t i = 0; i < poolConnections - 1; i++) {
        futures.emplace_back(utils::Async("LessQueriesThanConnections", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, EqualQueriesAndConnections) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}});

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections);

    for (std::size_t i = 0; i < poolConnections; i++) {
        futures.emplace_back(utils::Async("EqualQueriesAndConnections", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, MoreQueriesThanConnectionsButLessThanPoolSize) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections * 2}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}});

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections + 2);

    for (std::size_t i = 0; i < poolConnections + 2; i++) {
        futures.emplace_back(utils::Async("MoreQueriesThanConnectionsButLessThanPoolSize", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, MoreQueriesThanConnectionsAndPoolSize) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}});

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections * 2);

    for (std::size_t i = 0; i < poolConnections * 2; i++) {
        futures.emplace_back(utils::Async("MoreQueriesThanConnectionsAndPoolSize", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, RestoresBrokenConnection) {
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}});

    auto killConnectionQuery = "SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = 'postgres';";

    try {
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, killConnectionQuery);
    } catch (...) {
        // terminating the connection brokes query
    }

    auto selectRes = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
    EXPECT_EQ(selectRes.Size(), 1);
    EXPECT_EQ(selectRes[0][0].GetInt32(), 1);
}
}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
