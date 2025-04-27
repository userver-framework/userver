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
    userver::storages::odbc::settings::ODBCSettings settings{DSN};
    userver::storages::odbc::Connection conn(settings);
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END