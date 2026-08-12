#include <storages/odbc/detail/connection.hpp>

#include <chrono>
#include <cstdint>
#include <vector>

#include <fmt/format.h>
#include <sql.h>
#include <sqlext.h>

#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>

#include <storages/odbc/detail/broken_guard.hpp>
#include <storages/odbc/detail/deadline.hpp>
#include <storages/odbc/detail/diag_wrapper.hpp>
#include <storages/odbc/detail/result_wrapper.hpp>
#include <storages/odbc/detail/tracing.hpp>
#include <userver/storages/odbc/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

namespace {

void DestroyEnvironmentHandle(SQLHENV handle) {
    if (handle != SQL_NULL_HENV) {
        SQLFreeHandle(SQL_HANDLE_ENV, handle);
    }
}

void DestroyDatabaseHandle(SQLHDBC handle) {
    if (handle != SQL_NULL_HDBC) {
        SQLDisconnect(handle);
        SQLFreeHandle(SQL_HANDLE_DBC, handle);
    }
}

Connection::EnvironmentHandle MakeEnvironmentHandle() {
    SQLHENV env = SQL_NULL_HENV;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError(
            "Failed to allocate environment handle:" + detail::GetSQLDiagString(SQL_NULL_HANDLE, SQL_HANDLE_ENV)
        );
    }

    return Connection::EnvironmentHandle(env, &DestroyEnvironmentHandle);
}

Connection::DatabaseHandle MakeDatabaseHandle(SQLHENV env) {
    SQLHDBC dbc = SQL_NULL_HDBC;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError(
            "Failed to allocate connection handle:" + detail::GetSQLDiagString(SQL_NULL_HANDLE, SQL_HANDLE_DBC)
        );
    }

    return Connection::DatabaseHandle(dbc, &DestroyDatabaseHandle);
}

}  // namespace

Connection::Connection(const std::string& dsn)
    : env_(MakeEnvironmentHandle()),
      handle_(Connection::DatabaseHandle(SQL_NULL_HDBC, &DestroyDatabaseHandle))
{
    SQLRETURN ret =
        SQLSetEnvAttr(env_.get(), SQL_ATTR_CONNECTION_POOLING, reinterpret_cast<SQLPOINTER>(SQL_CP_ONE_PER_DRIVER), 0);
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError(
            "Failed to set connection pooling attribute:" + detail::GetSQLDiagString(env_.get(), SQL_HANDLE_ENV)
        );
    }

    ret = SQLSetEnvAttr(env_.get(), SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError("Failed to set ODBC version:" + detail::GetSQLDiagString(env_.get(), SQL_HANDLE_ENV));
    }

    handle_ = MakeDatabaseHandle(env_.get());

    std::vector<SQLCHAR> dsn_buffer(dsn.begin(), dsn.end());
    dsn_buffer.push_back('\0');
    ret =
        SQLDriverConnect(handle_.get(), nullptr, dsn_buffer.data(), SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_COMPLETE);
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError(
            "Failed to connect to database: " + detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
        );
    }

    SQLUINTEGER scroll_option = 0;
    ret = SQLGetInfo(handle_.get(), SQL_SCROLL_OPTIONS, &scroll_option, sizeof(scroll_option), nullptr);
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError(
            "Failed to get scroll options:" + detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
        );
    }

    // TODO: add support for other scroll options
    if (!(scroll_option & SQL_FD_FETCH_ABSOLUTE)) {
        throw ConnectionError("SQL_FD_FETCH_ABSOLUTE is not supported");
    }
}

ResultSet Connection::Query(std::string_view query) {
    return Query(query, detail::GetExecuteDeadline(detail::kDefaultStatementTimeout));
}

ResultSet Connection::Query(std::string_view query, engine::Deadline deadline) {
    detail::CheckDeadlineNotExpired(deadline);

    auto guard = GetBrokenGuard();
    return guard.Execute([&] {
        tracing::Span span{detail::tracing::MakeQuerySpanName(query)};
        span.AddTag(tracing::kDatabaseType, "odbc");
        span.AddTag(tracing::kDatabaseStatement, std::string{query});

        auto stmt = detail::MakeResultHandle(handle_.get());

        if (deadline.IsReachable()) {
            const auto left = deadline.TimeLeft();
            if (left > std::chrono::milliseconds::zero()) {
                const auto seconds = std::chrono::ceil<std::chrono::seconds>(left);
                const auto timeout_sec = static_cast<SQLULEN>(seconds.count());
                if (timeout_sec > 0) {
                    /* ODBC SQL_ATTR_QUERY_TIMEOUT is in whole seconds; deadline checks still use full TimeLeft()
                     * resolution. */
                    SQLSetStmtAttr(
                        stmt.get(),
                        SQL_ATTR_QUERY_TIMEOUT,
                        reinterpret_cast<SQLPOINTER>(static_cast<std::uintptr_t>(timeout_sec)),
                        0
                    );
                }
            }
        }

        std::vector<SQLCHAR> query_buffer(query.begin(), query.end());
        query_buffer.push_back('\0');
        SQLRETURN ret = SQLExecDirect(stmt.get(), query_buffer.data(), SQL_NTS);
        if (!SQL_SUCCEEDED(ret) && ret != SQL_NO_DATA) {
            const auto diag = detail::GetSQLDiagString(stmt.get(), SQL_HANDLE_STMT);
            span.AddTag(tracing::kErrorFlag, true);
            span.AddTag(tracing::kErrorMessage, diag);
            throw StatementError("Failed to execute query:" + diag);
        }

        auto wrapper = std::make_shared<detail::ResultWrapper>(std::move(stmt));
        // Only call Fetch for SELECT-like statements that produce a result set.
        // DML statements (INSERT/UPDATE/DELETE) have 0 result columns; calling
        // SQLFetch on them returns SQL_NO_DATA or an error depending on the driver.
        if (ret != SQL_NO_DATA) {
            SQLSMALLINT col_count = 0;
            SQLNumResultCols(stmt.get(), &col_count);
            if (col_count > 0) {
                wrapper->Fetch();
            }
        }

        return ResultSet(std::move(wrapper));
    });
}

bool Connection::DriverReportsDead() const {
    SQLUINTEGER state = 0;
    SQLRETURN ret = SQLGetConnectAttr(handle_.get(), SQL_ATTR_CONNECTION_DEAD, &state, sizeof(state), nullptr);
    if (!SQL_SUCCEEDED(ret) || state == SQL_CD_TRUE) {
        return true;
    }

    return false;
}

bool Connection::IsBroken() const { return broken_.load() || DriverReportsDead(); }

void Connection::NotifyBroken() { broken_.store(true); }

detail::BrokenGuard Connection::GetBrokenGuard() { return detail::BrokenGuard{*this}; }

bool Connection::IsInsideTransaction() const {
    SQLUINTEGER state = 0;
    SQLRETURN ret = SQLGetConnectAttr(handle_.get(), SQL_ATTR_AUTOCOMMIT, &state, sizeof(state), nullptr);
    if (!SQL_SUCCEEDED(ret) || state == SQL_AUTOCOMMIT_OFF) {
        return true;
    }
    return false;
}

void Connection::Begin(engine::Deadline deadline) {
    auto guard = GetBrokenGuard();
    guard.Execute([this, deadline] {
        detail::CheckDeadlineNotExpired(deadline);
        SQLRETURN ret = SQLSetConnectAttr(
            handle_.get(),
            SQL_ATTR_AUTOCOMMIT,
            reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_OFF),
            SQL_IS_UINTEGER
        );

        if (!SQL_SUCCEEDED(ret)) {
            throw ConnectionError(
                "Failed to set connection autocommit attribute:" +
                detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
            );
        }
    });
}

void Connection::Commit(engine::Deadline deadline) {
    auto guard = GetBrokenGuard();
    guard.Execute([this, deadline] {
        detail::CheckDeadlineNotExpired(deadline);
        if (!IsInsideTransaction()) {
            throw ConnectionError(
                "User try to commit autocommit connection:" + detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
            );
        }
        SQLRETURN ret = SQLEndTran(SQL_HANDLE_DBC, handle_.get(), SQL_COMMIT);
        if (!SQL_SUCCEEDED(ret)) {
            throw ConnectionError(
                "Failed to commit transaction inside connection:" +
                detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
            );
        }
        RestoreAutocommit();
    });
}

void Connection::Rollback(engine::Deadline deadline) {
    auto guard = GetBrokenGuard();
    guard.Execute([this, deadline] {
        detail::CheckDeadlineNotExpired(deadline);
        if (!IsInsideTransaction()) {
            throw ConnectionError(
                "User try to rollback autocommit connection:" + detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
            );
        }
        SQLRETURN ret = SQLEndTran(SQL_HANDLE_DBC, handle_.get(), SQL_ROLLBACK);
        if (!SQL_SUCCEEDED(ret)) {
            throw ConnectionError(
                "Failed to rollback transaction inside connection:" +
                detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
            );
        }
        RestoreAutocommit();
    });
}

void Connection::RestoreAutocommit() {
    SQLRETURN ret = SQLSetConnectAttr(
        handle_.get(),
        SQL_ATTR_AUTOCOMMIT,
        reinterpret_cast<SQLPOINTER>(SQL_AUTOCOMMIT_ON),
        SQL_IS_UINTEGER
    );
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError(
            "Failed to restore autocommit after transaction:" + detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
        );
    }
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
