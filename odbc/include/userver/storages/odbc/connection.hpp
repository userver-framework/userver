#pragma once

#include <string>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include <userver/storages/odbc/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace settings {
struct ODBCSettings {
    std::string DSN;
};
}  // namespace settings

/// @brief ODBC connection wrapper
class Connection final {
public:
    explicit Connection(const settings::ODBCSettings& settings);

    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    /// @brief Executes a SQL query and returns the result set
    /// @param query SQL query to execute
    /// @return ResultSet containing the query results
    ResultSet Query(const std::string& query);

private:
    SQLHENV env_;
    SQLHDBC handle_;
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
