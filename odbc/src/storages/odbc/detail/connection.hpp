#pragma once

#include <atomic>
#include <string>
#include <string_view>

#include <userver/engine/deadline.hpp>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include <userver/storages/odbc/impl/parameter.hpp>
#include <userver/storages/odbc/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace detail {
class BrokenGuard;
}

/// @brief ODBC connection wrapper
class Connection final {
public:
    using EnvironmentHandle = std::unique_ptr<std::remove_pointer_t<SQLHENV>, void (*)(SQLHENV)>;
    using DatabaseHandle = std::unique_ptr<std::remove_pointer_t<SQLHDBC>, void (*)(SQLHDBC)>;

    explicit Connection(const std::string& dsn);
    Connection(const std::string& dsn, engine::Deadline deadline);

    ~Connection() = default;

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    /// @brief Executes a SQL query and returns the result set
    /// @param query SQL query to execute
    /// @return ResultSet containing the query results
    ResultSet Query(std::string_view query);

    /// @brief Executes a prepared SQL query with separately bound parameters.
    ResultSet Query(std::string_view query, const impl::ParameterList& parameters);

    /// @brief Same as Query(std::string_view), but honours \a deadline for wait / driver timeout.
    ResultSet Query(std::string_view query, engine::Deadline deadline);

    ResultSet Query(std::string_view query, const impl::ParameterList& parameters, engine::Deadline deadline);

    // required by ConnectionPool
    bool IsBroken() const;
    void NotifyBroken();

    detail::BrokenGuard GetBrokenGuard();

private:
    friend class Transaction;
    void Begin(engine::Deadline deadline);
    void Commit(engine::Deadline deadline);
    void Rollback(engine::Deadline deadline);
    bool IsInsideTransaction() const;  // check if connection has autocommit_off transaction mode

    void RestoreAutocommit();
    bool DriverReportsDead() const;

    EnvironmentHandle env_;
    DatabaseHandle handle_;
    std::atomic<bool> broken_{false};
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
