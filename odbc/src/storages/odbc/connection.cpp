#include <sql.h>
#include <sqlext.h>
#include <stdexcept>
#include <vector>

#include <userver/storages/odbc/connection.hpp>
#include <userver/storages/odbc/odbc_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

Connection::Connection(const settings::ODBCSettings& settings) : env_(SQL_NULL_HENV), handle_(SQL_NULL_HDBC) {
    SQLRETURN ret =
        SQLSetEnvAttr(env_, SQL_ATTR_CONNECTION_POOLING, reinterpret_cast<SQLPOINTER>(SQL_CP_ONE_PER_DRIVER), 0);
    if (!SQL_SUCCEEDED(ret)) {
        throw std::runtime_error("Failed to allocate environment handle");
    }

    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env_);
    if (!SQL_SUCCEEDED(ret)) {
        throw std::runtime_error("Failed to allocate environment handle");
    }

    ret = SQLSetEnvAttr(env_, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
    if (!SQL_SUCCEEDED(ret)) {
        SQLFreeHandle(SQL_HANDLE_ENV, env_);
        throw std::runtime_error("Failed to set ODBC version");
    }

    ret = SQLAllocHandle(SQL_HANDLE_DBC, env_, &handle_);
    if (!SQL_SUCCEEDED(ret)) {
        SQLFreeHandle(SQL_HANDLE_ENV, env_);
        throw std::runtime_error("Failed to allocate connection handle");
    }

    std::vector<SQLCHAR> dsnBuffer(settings.DSN.begin(), settings.DSN.end());
    dsnBuffer.push_back('\0');
    ret = SQLDriverConnect(handle_, nullptr, dsnBuffer.data(), SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_COMPLETE);
    if (!SQL_SUCCEEDED(ret)) {
        SQLFreeHandle(SQL_HANDLE_DBC, handle_);
        SQLFreeHandle(SQL_HANDLE_ENV, env_);
        throw std::runtime_error("Failed to connect to database");
    }

    SQLUINTEGER scrollOption = 0;
    ret = SQLGetInfo(handle_, SQL_SCROLL_OPTIONS, &scrollOption, sizeof(scrollOption), nullptr);
    if (!SQL_SUCCEEDED(ret)) {
        SQLFreeHandle(SQL_HANDLE_DBC, handle_);
        SQLFreeHandle(SQL_HANDLE_ENV, env_);
        throw std::runtime_error("Failed to get scroll options");
    }

    // TODO: add support for other scroll options
    if (!(scrollOption & SQL_FD_FETCH_ABSOLUTE)) {
        SQLFreeHandle(SQL_HANDLE_DBC, handle_);
        SQLFreeHandle(SQL_HANDLE_ENV, env_);
        throw std::runtime_error("SQL_FD_FETCH_ABSOLUTE is not supported");
    }
}

Connection::~Connection() {
    if (handle_ != SQL_NULL_HDBC) {
        SQLDisconnect(handle_);
        SQLFreeHandle(SQL_HANDLE_DBC, handle_);
    }
    if (env_ != SQL_NULL_HENV) {
        SQLFreeHandle(SQL_HANDLE_ENV, env_);
    }
}

namespace {

detail::ResultWrapper FetchResultSet(SQLHSTMT stmt) { return detail::ResultWrapper(detail::MakeResultHandle(stmt)); }

}  // namespace

ResultSet Connection::Query(const std::string& query) {
    SQLHSTMT stmt = nullptr;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, handle_, &stmt);
    if (!SQL_SUCCEEDED(ret)) {
        throw std::runtime_error("Failed to allocate statement handle");
    }

    ret = SQLSetStmtAttr(stmt, SQL_ATTR_CURSOR_TYPE, reinterpret_cast<SQLPOINTER>(SQL_CURSOR_DYNAMIC), 0);
    if (!SQL_SUCCEEDED(ret)) {
        throw std::runtime_error("Failed to set cursor type");
    }

    std::vector<SQLCHAR> query_buffer(query.begin(), query.end());
    query_buffer.push_back('\0');
    ret = SQLExecDirect(stmt, query_buffer.data(), SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        throw std::runtime_error("Failed to execute query");
    }

    auto wrapper = std::make_shared<detail::ResultWrapper>(FetchResultSet(stmt));
    return ResultSet(std::move(wrapper));
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END