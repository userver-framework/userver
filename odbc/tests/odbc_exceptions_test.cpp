#include <gtest/gtest.h>
#include <userver/storages/odbc.hpp>
#include <userver/storages/odbc/tests/utils.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

UTEST(ConnectionError, InvalidDSN) {
    UEXPECT_THROW(
        storages::odbc::Cluster(
            storages::odbc::settings::ODBCClusterSettings{
                {storages::odbc::settings::HostSettings{"invalid_dsn", {5, 10}}}
            },
            nullptr
        )
            .Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1"),
        storages::odbc::ConnectionError
    );
}

UTEST(ConnectionError, InvalidCredentials) {
    UEXPECT_THROW(
        storages::odbc::Cluster(
            storages::odbc::settings::ODBCClusterSettings{{storages::odbc::settings::HostSettings{
                "DRIVER={PostgreSQL Unicode};"
                "SERVER=localhost;"
                "PORT=15433;"
                "DATABASE=postgres;"
                "UID=invalid_user;"
                "PWD=invalid_password;",
                {5, 10}
            }}},
            nullptr
        )
            .Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1"),
        storages::odbc::ConnectionError
    );
}

UTEST(StatementError, QueryingUnexistentTable) {
    auto cluster = MakeCluster();

    UEXPECT_THROW(
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT * FROM some_table"),
        storages::odbc::StatementError
    );
}

UTEST(StatementError, InvalidSyntax) {
    auto cluster = MakeCluster();

    UEXPECT_THROW(cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELEC 1"), storages::odbc::StatementError);
}

UTEST(StatementError, InvalidColumnReference) {
    auto cluster = MakeCluster();

    UEXPECT_THROW(
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT nonexistent_column FROM pg_tables"),
        storages::odbc::StatementError
    );
}

UTEST(ResultSetError, GettingInvalidRowIndex) {
    auto cluster = MakeCluster();
    auto resultSet = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
    UASSERT_THROW_MSG(resultSet[1], storages::odbc::RowIndexOutOfBounds, "Row index 1 is out of bounds");
}

UTEST(ResultSetError, GettingInvalidFieldIndex) {
    auto cluster = MakeCluster();
    auto resultSet = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
    UASSERT_THROW_MSG(resultSet[0][1], storages::odbc::FieldIndexOutOfBounds, "Field index 1 is out of bounds");
}

UTEST(ResultSetError, TypeConversionError) {
    auto cluster = MakeCluster();
    auto resultSet = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 'not_a_number'");
    UEXPECT_THROW(resultSet[0][0].GetInt32(), storages::odbc::ResultSetError);
}

UTEST(ResultSetError, NullValueAccess) {
    auto cluster = MakeCluster();
    auto resultSet = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT NULL");
    EXPECT_TRUE(resultSet[0][0].IsNull());
    UEXPECT_THROW(resultSet[0][0].GetInt32(), storages::odbc::ResultSetError);
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
