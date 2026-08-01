#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <sqltypes.h>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

/// Fully materialized ODBC result. No ODBC handle is retained: this makes a
/// ResultSet independent from the connection and safe to read after the
/// connection has returned to its pool.
class ResultWrapper final {
public:
    struct Column final {
        std::string name;
        SQLSMALLINT type{};
    };

    struct Cell final {
        std::optional<std::string> value;
    };

    using Row = std::vector<Cell>;

    ResultWrapper(std::vector<Column> columns, std::vector<Row> rows, std::size_t rows_affected);

    std::size_t RowCount() const noexcept;
    std::size_t FieldCount() const noexcept;
    std::size_t RowsAffected() const noexcept;

    const std::string& GetFieldName(std::size_t col) const;
    SQLSMALLINT GetColumnType(std::size_t col) const;

    std::string GetString(std::size_t row, std::size_t col) const;
    std::int32_t GetInt32(std::size_t row, std::size_t col) const;
    std::int64_t GetInt64(std::size_t row, std::size_t col) const;
    double GetDouble(std::size_t row, std::size_t col) const;
    bool GetBool(std::size_t row, std::size_t col) const;
    bool IsFieldNull(std::size_t row, std::size_t col) const;

private:
    const Cell& GetCell(std::size_t row, std::size_t col) const;
    const std::string& GetValue(std::size_t row, std::size_t col) const;

    std::vector<Column> columns_;
    std::vector<Row> rows_;
    std::size_t rows_affected_{0};
};

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
