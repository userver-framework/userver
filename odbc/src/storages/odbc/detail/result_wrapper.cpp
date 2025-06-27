#include <fmt/format.h>
#include <sqlext.h>
#include <storages/odbc/detail/diag_wrapper.hpp>
#include <storages/odbc/detail/result_wrapper.hpp>
#include <userver/storages/odbc/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {
namespace {

void CheckStatus(SQLRETURN ret, SQLHANDLE handle, SQLSMALLINT type) {
    if (ret == SQL_SUCCESS) {
        return;
    }
    auto exceptionMessage = fmt::format("SQLFunctionFailed failed: {} {}", ret, GetSQLDiagString(handle, type));
    throw ResultSetError(std::move(exceptionMessage));
}

void DestroyResultHandle(SQLHSTMT handle) {
    if (handle != SQL_NULL_HSTMT) {
        SQLFreeHandle(SQL_HANDLE_STMT, handle);
    }
}
}  // namespace

ResultWrapper::ResultHandle MakeResultHandle(SQLHDBC handle) {
    SQLHSTMT stmt = nullptr;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, handle, &stmt);
    if (!SQL_SUCCEEDED(ret)) {
        throw ResultSetError("Failed to allocate statement handle");
    }
    auto resultHandle = ResultWrapper::ResultHandle{stmt, &DestroyResultHandle};

    ret = SQLSetStmtAttr(resultHandle.get(), SQL_ATTR_CURSOR_TYPE, reinterpret_cast<SQLPOINTER>(SQL_CURSOR_DYNAMIC), 0);
    if (!SQL_SUCCEEDED(ret)) {
        throw ResultSetError("Failed to set cursor type");
    }

    return resultHandle;
}

ResultWrapper::ResultWrapper(ResultHandle&& res) : handle_{std::move(res)} {}
ResultWrapper::~ResultWrapper() = default;

ResultWrapper::ResultWrapper(ResultWrapper&& other) noexcept = default;

SQLRETURN ResultWrapper::GetStatus() const {
    SQLRETURN ret = SQLMoreResults(handle_.get());
    return ret;
}

void ResultWrapper::Fetch() { CheckStatus(SQLFetch(handle_.get()), handle_.get(), SQL_HANDLE_STMT); }

std::size_t ResultWrapper::RowCount() const {
    // TODO: drivers may return -1 or 0 if rows are not fetched yet, overall implementation for select is
    // driver-dependent, needs checking
    SQLLEN rowCount = 0;
    CheckStatus(SQLRowCount(handle_.get(), &rowCount), handle_.get(), SQL_HANDLE_STMT);
    return static_cast<std::size_t>(rowCount);
}

std::size_t ResultWrapper::FieldCount() const {
    SQLSMALLINT fieldCount = 0;
    CheckStatus(SQLNumResultCols(handle_.get(), &fieldCount), handle_.get(), SQL_HANDLE_STMT);
    return static_cast<std::size_t>(fieldCount);
}

std::size_t ResultWrapper::RowsAffected() const { return RowCount(); }

std::string ResultWrapper::GetFieldName(std::size_t col) const {
    SQLCHAR name[1024];
    SQLLEN name_len = sizeof(name);
    CheckStatus(
        SQLDescribeCol(handle_.get(), col + 1, name, sizeof(name), nullptr, nullptr, nullptr, nullptr, nullptr),
        handle_.get(),
        SQL_HANDLE_STMT
    );
    return std::string(reinterpret_cast<char*>(name), name_len);
}

SQLSMALLINT ResultWrapper::GetColumnType(std::size_t col) const {
    SQLSMALLINT type = 0;
    CheckStatus(
        SQLDescribeCol(handle_.get(), col + 1, nullptr, 0, nullptr, &type, nullptr, nullptr, nullptr),
        handle_.get(),
        SQL_HANDLE_STMT
    );
    return type;
}

std::string ResultWrapper::GetString(std::size_t row, std::size_t col) const {
    SQLSMALLINT type = GetColumnType(col);
    if (type != SQL_CHAR && type != SQL_VARCHAR && type != SQL_LONGVARCHAR && type != SQL_WCHAR &&
        type != SQL_WVARCHAR && type != SQL_WLONGVARCHAR) {
        throw ResultSetError(fmt::format("Type mismatch: column {} is not a string type (SQL type: {})", col + 1, type)
        );
    }
    CheckStatus(SQLFetchScroll(handle_.get(), SQL_FETCH_ABSOLUTE, row + 1), handle_.get(), SQL_HANDLE_STMT);
    SQLCHAR value[1024];
    SQLLEN valueLen = sizeof(value);
    CheckStatus(
        SQLGetData(handle_.get(), col + 1, SQL_C_CHAR, value, sizeof(value), &valueLen), handle_.get(), SQL_HANDLE_STMT
    );
    return std::string(reinterpret_cast<char*>(value), valueLen);
}

std::int32_t ResultWrapper::GetInt32(std::size_t row, std::size_t col) const {
    SQLSMALLINT type = GetColumnType(col);
    if (type != SQL_INTEGER && type != SQL_SMALLINT && type != SQL_TINYINT) {
        throw ResultSetError(fmt::format("Type mismatch: column {} is not an int32 type (SQL type: {})", col, type));
    }
    CheckStatus(SQLFetchScroll(handle_.get(), SQL_FETCH_ABSOLUTE, row + 1), handle_.get(), SQL_HANDLE_STMT);
    SQLINTEGER value = 0;
    CheckStatus(SQLGetData(handle_.get(), col + 1, SQL_C_SLONG, &value, 0, nullptr), handle_.get(), SQL_HANDLE_STMT);
    return static_cast<std::int32_t>(value);
}

std::int64_t ResultWrapper::GetInt64(std::size_t row, std::size_t col) const {
    SQLSMALLINT type = GetColumnType(col);
    if (type != SQL_BIGINT) {
        throw ResultSetError(fmt::format("Type mismatch: column {} is not an int64 type (SQL type: {})", col, type));
    }
    CheckStatus(SQLFetchScroll(handle_.get(), SQL_FETCH_ABSOLUTE, row + 1), handle_.get(), SQL_HANDLE_STMT);
    SQLBIGINT value = 0;
    CheckStatus(SQLGetData(handle_.get(), col + 1, SQL_C_SBIGINT, &value, 0, nullptr), handle_.get(), SQL_HANDLE_STMT);
    return static_cast<std::int64_t>(value);
}

double ResultWrapper::GetDouble(std::size_t row, std::size_t col) const {
    SQLSMALLINT type = GetColumnType(col);
    if (type != SQL_DOUBLE && type != SQL_FLOAT && type != SQL_REAL && type != SQL_DECIMAL && type != SQL_NUMERIC) {
        throw ResultSetError(
            fmt::format("Type mismatch: column {} is not a floating-point type (SQL type: {})", col, type)
        );
    }
    CheckStatus(SQLFetchScroll(handle_.get(), SQL_FETCH_ABSOLUTE, row + 1), handle_.get(), SQL_HANDLE_STMT);
    SQLDOUBLE value = 0;
    CheckStatus(SQLGetData(handle_.get(), col + 1, SQL_C_DOUBLE, &value, 0, nullptr), handle_.get(), SQL_HANDLE_STMT);
    return static_cast<double>(value);
}

bool ResultWrapper::GetBool(std::size_t row, std::size_t col) const {
    // NOTE: typecheck for bools omitted, since drivers may handle bools differently, but always support SQL_C_BIT
    CheckStatus(SQLFetchScroll(handle_.get(), SQL_FETCH_ABSOLUTE, row + 1), handle_.get(), SQL_HANDLE_STMT);
    bool value = false;
    CheckStatus(
        SQLGetData(handle_.get(), col + 1, SQL_C_BIT, &value, sizeof(value), nullptr), handle_.get(), SQL_HANDLE_STMT
    );
    return value;
}

bool ResultWrapper::IsFieldNull(std::size_t row, std::size_t col) const {
    CheckStatus(SQLFetchScroll(handle_.get(), SQL_FETCH_ABSOLUTE, row + 1), handle_.get(), SQL_HANDLE_STMT);
    SQLLEN marker = 0;
    bool dummy = false;  // NOTE: odbc requires a buffer for SQL_C_DEFAULT
    CheckStatus(
        SQLGetData(handle_.get(), col + 1, SQL_C_DEFAULT, &dummy, sizeof(dummy), &marker),
        handle_.get(),
        SQL_HANDLE_STMT
    );
    return marker == SQL_NULL_DATA;
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
