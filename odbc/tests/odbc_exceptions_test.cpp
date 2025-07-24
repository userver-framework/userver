#include <gtest/gtest.h>
#include <userver/storages/odbc.hpp>
#include <userver/utest/utest.hpp>

#include <iostream>

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

UTEST(ConnectionError, InvalidDSN) {
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{
        {storages::odbc::settings::HostSettings{"invalid_dsn", {5, 10}}}});

    UEXPECT_THROW(
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1"), storages::odbc::ConnectionError
    );
}

UTEST(ConnectionError, InvalidCredentials) {
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{
        {storages::odbc::settings::HostSettings{
            "DRIVER={PostgreSQL Unicode};"
            "SERVER=localhost;"
            "PORT=15433;"
            "DATABASE=postgres;"
            "UID=invalid_user;"
            "PWD=invalid_password;",
            {5, 10}}}});

    UEXPECT_THROW(
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1"), storages::odbc::ConnectionError
    );
}

UTEST(StatementError, QueryingUnexistentTable) {
    storages::odbc::Cluster cluster(kSettings);

    UEXPECT_THROW(
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT * FROM some_table"),
        storages::odbc::StatementError
    );
}

UTEST(StatementError, InvalidSyntax) {
    storages::odbc::Cluster cluster(kSettings);

    UEXPECT_THROW(cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELEC 1"), storages::odbc::StatementError);
}

UTEST(StatementError, InvalidColumnReference) {
    storages::odbc::Cluster cluster(kSettings);

    UEXPECT_THROW(
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT nonexistent_column FROM pg_tables"),
        storages::odbc::StatementError
    );
}

UTEST(ResultSetError, GettingInvalidRowIndex) {
    storages::odbc::Cluster cluster(kSettings);
    auto resultSet = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
    UASSERT_THROW_MSG(resultSet[1], storages::odbc::RowIndexOutOfBounds, "Row index 1 is out of bounds");
}

UTEST(ResultSetError, GettingInvalidFieldIndex) {
    storages::odbc::Cluster cluster(kSettings);
    auto resultSet = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
    UASSERT_THROW_MSG(resultSet[0][1], storages::odbc::FieldIndexOutOfBounds, "Field index 1 is out of bounds");
}

UTEST(ResultSetError, TypeConversionError) {
    storages::odbc::Cluster cluster(kSettings);
    auto resultSet = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 'not_a_number'");
    UEXPECT_THROW(resultSet[0][0].GetInt32(), storages::odbc::ResultSetError);
}

UTEST(ResultSetError, NullValueAccess) {
    storages::odbc::Cluster cluster(kSettings);
    auto resultSet = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT NULL");
    EXPECT_TRUE(resultSet[0][0].IsNull());
    UEXPECT_THROW(resultSet[0][0].GetInt32(), storages::odbc::ResultSetError);
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
