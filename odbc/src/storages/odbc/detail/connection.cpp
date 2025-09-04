#include <storages/odbc/detail/connection.hpp>

#include <storages/odbc/detail/diag_wrapper.hpp>
#include <userver/storages/odbc/exception.hpp>

#include <vector>

#include <fmt/format.h>
#include <sql.h>
#include <sqlext.h>

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
    : env_(MakeEnvironmentHandle()), handle_(Connection::DatabaseHandle(SQL_NULL_HDBC, &DestroyDatabaseHandle)) {
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

    std::vector<SQLCHAR> dsnBuffer(dsn.begin(), dsn.end());
    dsnBuffer.push_back('\0');
    ret = SQLDriverConnect(handle_.get(), nullptr, dsnBuffer.data(), SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_COMPLETE);
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError(
            "Failed to connect to database: " + detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
        );
    }

    SQLUINTEGER scrollOption = 0;
    ret = SQLGetInfo(handle_.get(), SQL_SCROLL_OPTIONS, &scrollOption, sizeof(scrollOption), nullptr);
    if (!SQL_SUCCEEDED(ret)) {
        throw ConnectionError(
            "Failed to get scroll options:" + detail::GetSQLDiagString(handle_.get(), SQL_HANDLE_DBC)
        );
    }

    // TODO: add support for other scroll options
    if (!(scrollOption & SQL_FD_FETCH_ABSOLUTE)) {
        throw ConnectionError("SQL_FD_FETCH_ABSOLUTE is not supported");
    }
}

ResultSet Connection::Query(std::string_view query) {
    auto stmt = detail::MakeResultHandle(handle_.get());

    std::vector<SQLCHAR> query_buffer(query.begin(), query.end());
    query_buffer.push_back('\0');
    SQLRETURN ret = SQLExecDirect(stmt.get(), query_buffer.data(), SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        throw StatementError("Failed to execute query:" + detail::GetSQLDiagString(stmt.get(), SQL_HANDLE_STMT));
    }

    auto wrapper = std::make_shared<detail::ResultWrapper>(std::move(stmt));
    wrapper->Fetch();

    return ResultSet(std::move(wrapper));
}

bool Connection::IsBroken() const {
    SQLUINTEGER state = 0;
    SQLRETURN ret = SQLGetConnectAttr(handle_.get(), SQL_ATTR_CONNECTION_DEAD, &state, sizeof(state), nullptr);
    if (!SQL_SUCCEEDED(ret) || state == SQL_CD_TRUE) {
        return true;
    }

    return false;
}

void Connection::NotifyBroken() {}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
