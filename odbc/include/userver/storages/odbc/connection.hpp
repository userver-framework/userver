#pragma once

#include <string>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include <userver/storages/odbc/command_result_set.hpp>
#include <userver/storages/odbc/statement_result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace settings {
struct ODBCSettings {
    std::string DSN;
};
}  // namespace settings

class Connection final {
public:
    explicit Connection(const settings::ODBCSettings& settings);

    ~Connection();

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    /// @brief Executes a SQL query and returns the result set
    /// @param query SQL query to execute
    /// @return CommandResultSet containing the query results
    CommandResultSet Query(const std::string& query);

    /// @brief Executes a statement and returns the result set
    /// @param query SQL query to prepare and execute
    /// @return StatementResultSet containing the query results
    StatementResultSet Execute(const std::string& query);

private:
    SQLHENV env_;
    SQLHDBC handle_;
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
