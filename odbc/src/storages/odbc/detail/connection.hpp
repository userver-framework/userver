#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <string_view>

#include <userver/engine/deadline.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include <userver/storages/odbc/impl/parameter.hpp>
#include <userver/storages/odbc/query.hpp>
#include <userver/storages/odbc/result_set.hpp>

#include <storages/odbc/detail/driver_capabilities.hpp>

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
    Connection(const std::string& dsn, engine::TaskProcessor& blocking_task_processor, engine::Deadline deadline);

    ~Connection();

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

    ResultSet Query(
        const storages::odbc::Query& query,
        const impl::ParameterList& parameters,
        engine::Deadline deadline
    );

    // required by ConnectionPool
    bool IsBroken() const;
    bool IsMarkedBroken() const noexcept;
    void NotifyBroken();

    detail::BrokenGuard GetBrokenGuard();

    /// Internal per-HDBC metadata snapshot captured during construction.
    const detail::DriverCapabilities& GetDriverCapabilities() const noexcept;

private:
    friend class Transaction;
    void Begin(engine::Deadline deadline);
    void Commit(engine::Deadline deadline);
    void Rollback(engine::Deadline deadline);
    bool IsInsideTransaction() const noexcept;

    void RestoreAutocommit();
    void UpdateBrokenFromDriver() noexcept;
    engine::TaskProcessor& blocking_task_processor_;
    mutable std::mutex handle_mutex_;
    EnvironmentHandle env_;
    DatabaseHandle handle_;
    detail::DriverCapabilities driver_capabilities_;
    std::atomic<bool> broken_{false};
    std::atomic<bool> in_transaction_{false};
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
