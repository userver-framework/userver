#include <userver/storages/odbc.hpp>
#include <userver/utest/utest.hpp>

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
auto kSettings = storages::odbc::settings::ODBCClusterSettings{{kDSN}};
}

UTEST(CreateConnection, Works) { storages::odbc::Cluster cluster(kSettings); }

UTEST(CreateConnection, MultipleDSN) {
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{kDSN, kDSN}});
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
    auto query = "SELECT 42, 'test', 1.0, false, null";
    storages::odbc::Cluster cluster(kSettings);

    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, query);
    EXPECT_EQ(result.Size(), 1);
    EXPECT_FALSE(result.IsEmpty());
    auto row = result[0];
    EXPECT_EQ(row.Size(), 5);

    auto intField = row[0];
    EXPECT_EQ(intField.GetInt32(), 42);

    auto stringField = row[1];
    EXPECT_EQ(stringField.GetString(), "test");

    auto doubleField = row[2];
    EXPECT_DOUBLE_EQ(doubleField.GetDouble(), 1.0);

    auto boolField = row[3];
    EXPECT_EQ(boolField.GetInt32(), 0);

    auto nullField = row[4];
    EXPECT_TRUE(nullField.IsNull());
}

UTEST(Query, DifferentHostTypes) {
    auto query = "SELECT 1";
    storages::odbc::Cluster cluster(kSettings);

    // TODO: needs an actual check that host are selected correctly
    cluster.Execute(storages::odbc::ClusterHostType::kMaster, query);
    cluster.Execute(storages::odbc::ClusterHostType::kSlave, query);
    cluster.Execute(storages::odbc::ClusterHostType::kNone, query);
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
