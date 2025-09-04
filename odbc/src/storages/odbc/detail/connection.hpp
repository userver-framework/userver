#pragma once

#include <string>
#include <string_view>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include <userver/storages/odbc/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

/// @brief ODBC connection wrapper
class Connection final {
public:
    using EnvironmentHandle = std::unique_ptr<std::remove_pointer_t<SQLHENV>, void (*)(SQLHENV)>;
    using DatabaseHandle = std::unique_ptr<std::remove_pointer_t<SQLHDBC>, void (*)(SQLHDBC)>;

    explicit Connection(const std::string& dsn);

    ~Connection() = default;

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    /// @brief Executes a SQL query and returns the result set
    /// @param query SQL query to execute
    /// @return ResultSet containing the query results
    ResultSet Query(std::string_view query);

    // required by ConnectionPool
    bool IsBroken() const;
    void NotifyBroken();

private:
    EnvironmentHandle env_;
    DatabaseHandle handle_;
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
