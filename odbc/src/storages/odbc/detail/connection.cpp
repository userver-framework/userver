#include <sql.h>
#include <sqlext.h>
#include <stdexcept>
#include <vector>

#include <storages/odbc/detail/connection.hpp>

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
        throw std::runtime_error("Failed to allocate environment handle");
    }

    return Connection::EnvironmentHandle(env, &DestroyEnvironmentHandle);
}

Connection::DatabaseHandle MakeDatabaseHandle(SQLHENV env) {
    SQLHDBC dbc = SQL_NULL_HDBC;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);
    if (!SQL_SUCCEEDED(ret)) {
        throw std::runtime_error("Failed to allocate connection handle");
    }

    return Connection::DatabaseHandle(dbc, &DestroyDatabaseHandle);
}

}  // namespace

Connection::Connection(const std::string& dsn)
    : env_(MakeEnvironmentHandle()), handle_(Connection::DatabaseHandle(SQL_NULL_HDBC, &DestroyDatabaseHandle)) {
    SQLRETURN ret =
        SQLSetEnvAttr(env_.get(), SQL_ATTR_CONNECTION_POOLING, reinterpret_cast<SQLPOINTER>(SQL_CP_ONE_PER_DRIVER), 0);
    if (!SQL_SUCCEEDED(ret)) {
        throw std::runtime_error("Failed to set connection pooling attribute");
    }

    ret = SQLSetEnvAttr(env_.get(), SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);
    if (!SQL_SUCCEEDED(ret)) {
        throw std::runtime_error("Failed to set ODBC version");
    }

    handle_ = MakeDatabaseHandle(env_.get());

    std::vector<SQLCHAR> dsnBuffer(dsn.begin(), dsn.end());
    dsnBuffer.push_back('\0');
    ret = SQLDriverConnect(handle_.get(), nullptr, dsnBuffer.data(), SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_COMPLETE);
    if (!SQL_SUCCEEDED(ret)) {
        throw std::runtime_error("Failed to connect to database");
    }

    SQLUINTEGER scrollOption = 0;
    ret = SQLGetInfo(handle_.get(), SQL_SCROLL_OPTIONS, &scrollOption, sizeof(scrollOption), nullptr);
    if (!SQL_SUCCEEDED(ret)) {
        throw std::runtime_error("Failed to get scroll options");
    }

    // TODO: add support for other scroll options
    if (!(scrollOption & SQL_FD_FETCH_ABSOLUTE)) {
        throw std::runtime_error("SQL_FD_FETCH_ABSOLUTE is not supported");
    }
}

ResultSet Connection::Query(const std::string& query) {
    auto stmt = detail::MakeResultHandle(handle_.get());

    std::vector<SQLCHAR> query_buffer(query.begin(), query.end());
    query_buffer.push_back('\0');
    SQLRETURN ret = SQLExecDirect(stmt.get(), query_buffer.data(), SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        throw std::runtime_error("Failed to execute query");
    }

    auto wrapper = std::make_shared<detail::ResultWrapper>(std::move(stmt));
    wrapper->Fetch();

    return ResultSet(std::move(wrapper));
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
