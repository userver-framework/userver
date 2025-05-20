#pragma once

#include <string>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include <userver/storages/odbc/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

/// @brief ODBC connection wrapper
class Connection final {
public:
    explicit Connection(const std::string& dsn);

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
