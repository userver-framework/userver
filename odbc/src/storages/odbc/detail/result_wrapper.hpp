#pragma once

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include <functional>
#include <memory>
#include <string>
#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

class ResultWrapper {
public:
    using ResultHandle = std::unique_ptr<std::remove_pointer_t<SQLHSTMT>, std::function<void(SQLHSTMT)>>;

    ResultWrapper(ResultHandle&& res);
    ~ResultWrapper();

    ResultWrapper(const ResultWrapper&) = delete;
    ResultWrapper(ResultWrapper&& other) noexcept;

    SQLRETURN GetStatus() const;

    std::size_t RowCount() const;
    std::size_t FieldCount() const;
    std::size_t RowsAffected() const;

    std::string GetFieldName(std::size_t col) const;
    SQLSMALLINT GetColumnType(std::size_t col) const;
    
    // Data access methods
    std::string GetString(std::size_t row, std::size_t col) const;
    std::int32_t GetInt32(std::size_t row, std::size_t col) const;
    std::int64_t GetInt64(std::size_t row, std::size_t col) const;
    double GetDouble(std::size_t row, std::size_t col) const;

    bool IsFieldNull(std::size_t row, std::size_t col) const;

    ResultHandle handle_;
};

inline ResultWrapper::ResultHandle MakeResultHandle(SQLHSTMT stmt) {
    auto deleter = [](SQLHSTMT handle) { SQLFreeStmt(handle, SQL_CLOSE); };
    return {stmt, deleter};
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
