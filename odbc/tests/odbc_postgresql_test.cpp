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
    storages::odbc::settings::ODBCSettings settings{DSN};
    storages::odbc::Connection conn(settings);
}

UTEST(Query, Works) {
    std::string DSN =
        "DRIVER={PostgreSQL Unicode};"
        "SERVER=localhost;"
        "PORT=15433;"
        "DATABASE=postgres;"
        "UID=testsuite;"
        "PWD=password;";
    storages::odbc::settings::ODBCSettings settings{DSN};
    storages::odbc::Connection conn(settings);

    auto result = conn.Query("SELECT 1");
    EXPECT_EQ(result.RowsCount(), 1);
    EXPECT_EQ(result.At(0, 0), "1");

    auto multipleRows = conn.Query("SELECT generate_series(1, 10)");
    EXPECT_EQ(multipleRows.RowsCount(), 10);
    for (std::size_t i = 0; i < multipleRows.RowsCount(); i++) {
        EXPECT_EQ(multipleRows.At(i, 0), std::to_string(i + 1));
    }
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END