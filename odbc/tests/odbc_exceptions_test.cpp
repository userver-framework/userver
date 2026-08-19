#include <gtest/gtest.h>

#include <string_view>
#include <vector>

#include <storages/odbc/detail/diag_wrapper.hpp>
#include <storages/odbc/odbc_secdist.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/storages/odbc.hpp>
#include <userver/storages/odbc/tests/utils.hpp>
#include <userver/storages/secdist/exceptions.hpp>
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
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);
    cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "CREATE TEMP TABLE odbc_statement_error_session_marker(value INTEGER)"
    );
    cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "INSERT INTO odbc_statement_error_session_marker(value) VALUES (1)"
    );

    try {
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELEC 1");
        FAIL() << "Invalid SQL must throw";
    } catch (const storages::odbc::StatementError& ex) {
        ASSERT_FALSE(ex.GetDiagnostics().empty());
        EXPECT_EQ(ex.GetDiagnostics().front().sql_state, "42601");
        EXPECT_TRUE(ex.HasSqlStateClass("42"));
        EXPECT_FALSE(ex.HasSqlStateClass("08"));
        EXPECT_NE(std::string_view{ex.what()}.find("[42601]"), std::string_view::npos);
    }

    // A statement-level error must not evict or poison an otherwise healthy HDBC.
    const auto result =
        cluster
            .Execute(storages::odbc::ClusterHostType::kMaster, "SELECT value FROM odbc_statement_error_session_marker");
    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetInt32(), 1);
}

UTEST(StatementError, ClassifiesConnectionDiagnostics) {
    const std::vector<DiagnosticRecord> diagnostics{
        DiagnosticRecord{.sql_state = "01004", .native_error = 0, .message = "truncated"},
        DiagnosticRecord{.sql_state = "08006", .native_error = 7, .message = "connection failure"},
    };

    EXPECT_TRUE(detail::HasConnectionError(diagnostics));
    const StatementError error{"driver call failed", diagnostics, false};
    EXPECT_TRUE(error.HasSqlStateClass("08"));
    EXPECT_FALSE(error.IsInvalidHandle());

    const auto formatted = detail::FormatSQLDiagnostics(diagnostics);
    EXPECT_NE(formatted.find("[01004] truncated (native code 0)"), std::string::npos);
    EXPECT_NE(formatted.find("[08006] connection failure (native code 7)"), std::string::npos);
}

UTEST(OdbcSecdist, RejectsAmbiguousConnectionSource) {
    const auto doc = formats::json::FromString(R"({
        "odbc_settings": {"databases": {"test": {
            "dsn": "dsn-1",
            "hosts": ["dsn-2"]
        }}}
    })");
    UEXPECT_THROW(secdist::OdbcSettings{doc}, storages::secdist::SecdistError);
}

UTEST(OdbcSecdist, RejectsMalformedHostObject) {
    const auto doc = formats::json::FromString(R"({
        "odbc_settings": {"databases": {"test": {
            "hosts": [{"dsn": "dsn-1", "unexpected": true}]
        }}}
    })");
    UEXPECT_THROW(secdist::OdbcSettings{doc}, storages::secdist::SecdistError);
}

UTEST(OdbcSecdist, RejectsEmptyDsn) {
    const auto direct = formats::json::FromString(R"({
        "odbc_settings": {"databases": {"test": {"dsn": ""}}}
    })");
    UEXPECT_THROW(secdist::OdbcSettings{direct}, storages::secdist::SecdistError);

    const auto in_hosts = formats::json::FromString(R"({
        "odbc_settings": {"databases": {"test": {"hosts": [""]}}}
    })");
    UEXPECT_THROW(secdist::OdbcSettings{in_hosts}, storages::secdist::SecdistError);
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
