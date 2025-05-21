#include <storages/odbc/detail/result_wrapper.hpp>

#include "userver/storages/odbc/exception.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {
namespace {
void CheckStatus(SQLRETURN ret) {
    if (ret == SQL_SUCCESS) {
        return;
    }
    throw ResultSetError("SQLFunctionFailed failed: " + std::to_string(ret));
}
}  // namespace

ResultWrapper::ResultWrapper(ResultHandle&& res) : handle_{std::move(res)} { CheckStatus(SQLFetch(handle_.get())); }
ResultWrapper::~ResultWrapper() { SQLFreeStmt(handle_.get(), SQL_CLOSE); }

ResultWrapper::ResultWrapper(ResultWrapper&& other) noexcept = default;

SQLRETURN ResultWrapper::GetStatus() const {
    SQLRETURN ret = SQLMoreResults(handle_.get());
    return ret;
}

std::size_t ResultWrapper::RowCount() const {
    // TODO: drivers may return -1 or 0 if rows are not fetched yet, overall implementation for select is driver-dependent, needs checking
    SQLLEN rowCount = 0;
    CheckStatus(SQLRowCount(handle_.get(), &rowCount));
    return static_cast<std::size_t>(rowCount);
}

std::size_t ResultWrapper::FieldCount() const {
    SQLSMALLINT fieldCount = 0;
    CheckStatus(SQLNumResultCols(handle_.get(), &fieldCount));
    return static_cast<std::size_t>(fieldCount);
}

std::size_t ResultWrapper::RowsAffected() const { return RowCount(); }

std::string ResultWrapper::GetFieldName(std::size_t col) const {
    SQLCHAR name[1024];
    SQLLEN name_len = sizeof(name);
    CheckStatus(SQLDescribeCol(handle_.get(), col + 1, name, sizeof(name), nullptr, nullptr, nullptr, nullptr, nullptr)
    );
    return std::string(reinterpret_cast<char*>(name), name_len);
}

SQLSMALLINT ResultWrapper::GetColumnType(std::size_t col) const {
    SQLSMALLINT type = 0;
    CheckStatus(SQLDescribeCol(handle_.get(), col + 1, nullptr, 0, nullptr, nullptr, nullptr, nullptr, nullptr));
    return type;
}

std::string ResultWrapper::GetString(std::size_t row, std::size_t col) const {
    CheckStatus(SQLFetchScroll(handle_.get(), SQL_FETCH_ABSOLUTE, row + 1));
    SQLCHAR value[1024];
    SQLLEN valueLen = sizeof(value);
    CheckStatus(SQLGetData(handle_.get(), col + 1, SQL_C_CHAR, value, sizeof(value), &valueLen));
    return std::string(reinterpret_cast<char*>(value), valueLen);
}

std::int32_t ResultWrapper::GetInt32(std::size_t row, std::size_t col) const {
    CheckStatus(SQLFetchScroll(handle_.get(), SQL_FETCH_ABSOLUTE, row + 1));
    SQLINTEGER value = 0;
    CheckStatus(SQLGetData(const_cast<SQLHSTMT>(handle_.get()), col + 1, SQL_C_SLONG, &value, 0, nullptr));
    return static_cast<std::int32_t>(value);
}

std::int64_t ResultWrapper::GetInt64(std::size_t row, std::size_t col) const {
    CheckStatus(SQLFetchScroll(handle_.get(), SQL_FETCH_ABSOLUTE, row + 1));
    SQLBIGINT value = 0;
    CheckStatus(SQLGetData(handle_.get(), col + 1, SQL_C_SBIGINT, &value, 0, nullptr));
    return static_cast<std::int64_t>(value);
}

double ResultWrapper::GetDouble(std::size_t row, std::size_t col) const {
    CheckStatus(SQLFetchScroll(handle_.get(), SQL_FETCH_ABSOLUTE, row + 1));
    SQLDOUBLE value = 0;
    CheckStatus(SQLGetData(handle_.get(), col + 1, SQL_C_DOUBLE, &value, 0, nullptr));
    return static_cast<double>(value);
}

bool ResultWrapper::IsFieldNull(std::size_t row, std::size_t col) const {
    CheckStatus(SQLFetchScroll(handle_.get(), SQL_FETCH_ABSOLUTE, row + 1));
    SQLLEN marker = 0;
    bool dummy = false; // NOTE: odbc requires a buffer for SQL_C_DEFAULT
    CheckStatus(SQLGetData(handle_.get(), col + 1, SQL_C_DEFAULT, &dummy, sizeof(dummy), &marker));
    return marker == SQL_NULL_DATA;
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END