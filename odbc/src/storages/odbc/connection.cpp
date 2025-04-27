#include <sql.h>
#include <sqlext.h>
#include <stdexcept>

#include <userver/storages/odbc/connection.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

Connection::Connection(const settings::ODBCSettings& settings) : env_(), handle_() {
    auto sqlret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env_);
    if (!SQL_SUCCEEDED(sqlret)) {
        throw std::runtime_error("failed to alloc handle: " + std::to_string(sqlret));
    }
    SQLSetEnvAttr(env_, SQL_ATTR_ODBC_VERSION, reinterpret_cast<void*>(SQL_OV_ODBC3), 0);
    SQLAllocHandle(SQL_HANDLE_DBC, env_, &handle_);
    SQLCHAR* dsn_ptr = reinterpret_cast<SQLCHAR*>(const_cast<char*>(settings.DSN.c_str()));  // NOLINT
    auto ret = SQLDriverConnect(
        handle_, nullptr, dsn_ptr, SQL_NTS, nullptr, 0, nullptr, SQL_DRIVER_NOPROMPT
    );

    if (!SQL_SUCCEEDED(ret)) {
        throw std::runtime_error("failed to establish connection: " + std::to_string(ret));
    }
}

Connection::~Connection() {
    SQLFreeEnv(env_);
    SQLFreeHandle(SQL_HANDLE_ENV, handle_);
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END