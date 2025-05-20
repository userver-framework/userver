#include <userver/storages/odbc.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

UTEST(CreateConnection, Works) {
    std::string DSN =
        "DRIVER={PostgreSQL Unicode};"
        "SERVER=localhost;"
        "PORT=15433;"
        "DATABASE=postgres;"
        "UID=testsuite;"
        "PWD=password;";
    storages::odbc::settings::ODBCClusterSettings settings{{DSN}};
    storages::odbc::Cluster cluster(settings);
}

UTEST(Query, Works) {
    std::string DSN =
        "DRIVER={PostgreSQL Unicode};"
        "SERVER=localhost;"
        "PORT=15433;"
        "DATABASE=postgres;"
        "UID=testsuite;"
        "PWD=password;";
    storages::odbc::settings::ODBCClusterSettings settings{{DSN}};
    storages::odbc::Cluster cluster(settings);

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

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END