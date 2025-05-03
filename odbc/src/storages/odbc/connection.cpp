#include <sql.h>
#include <sqlext.h>
#include <stdexcept>
#include <vector>

#include <storages/odbc/impl/query_result.hpp>
#include <userver/storages/odbc/connection.hpp>

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

impl::QueryResult FetchResultSet(SQLHSTMT stmt) {
    impl::QueryResult result;
    SQLSMALLINT columns_count = 0;
    SQLRETURN ret = SQLNumResultCols(stmt, &columns_count);
    if (!SQL_SUCCEEDED(ret)) {
        throw std::runtime_error("Failed to get columns count");
    }

    while (true) {
        ret = SQLFetch(stmt);
        if (ret == SQL_NO_DATA) break;
        if (!SQL_SUCCEEDED(ret)) {
            throw std::runtime_error("Failed to fetch row");
        }

        impl::QueryResultRow row;
        for (SQLSMALLINT i = 1; i <= columns_count; ++i) {
            SQLCHAR buffer[4096];
            SQLLEN indicator = 0;
            ret = SQLGetData(stmt, i, SQL_C_CHAR, buffer, sizeof(buffer), &indicator);
            if (!SQL_SUCCEEDED(ret)) {
                throw std::runtime_error("Failed to get column data");
            }

            if (indicator == SQL_NULL_DATA) {
                row.AppendField(std::string{});
            } else {
                row.AppendField(std::string(reinterpret_cast<char*>(buffer), indicator));
            }
        }
        result.AppendRow(std::move(row));
    }

    return result;
}

}  // namespace

CommandResultSet Connection::Query(const std::string& query) {
    SQLHSTMT stmt = nullptr;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, handle_, &stmt);
    if (!SQL_SUCCEEDED(ret)) {
        throw std::runtime_error("Failed to allocate statement handle");
    }

    std::vector<SQLCHAR> query_buffer(query.begin(), query.end());
    query_buffer.push_back('\0');
    ret = SQLExecDirect(stmt, query_buffer.data(), SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        throw std::runtime_error("Failed to execute query");
    }

    auto result = FetchResultSet(stmt);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return CommandResultSet(std::move(result));
}

StatementResultSet Connection::Execute(const std::string& query) {
    SQLHSTMT stmt = nullptr;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, handle_, &stmt);
    if (!SQL_SUCCEEDED(ret)) {
        throw std::runtime_error("Failed to allocate statement handle");
    }

    std::vector<SQLCHAR> queryBuffer(query.begin(), query.end());
    queryBuffer.push_back('\0');
    ret = SQLPrepare(stmt, queryBuffer.data(), SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        throw std::runtime_error("Failed to prepare statement");
    }

    ret = SQLExecute(stmt);
    if (!SQL_SUCCEEDED(ret)) {
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        throw std::runtime_error("Failed to execute prepared statement");
    }

    auto result = FetchResultSet(stmt);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return StatementResultSet(std::move(result));
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END