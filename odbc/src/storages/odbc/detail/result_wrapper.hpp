#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <sqlext.h>

#include <userver/storages/odbc/types.hpp>

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
        SQLULEN size{};
        SQLSMALLINT decimal_digits{};
    };

    struct DecimalValue final {
        std::string representation;
        std::uint8_t precision{};
        std::uint8_t scale{};
    };

    struct Cell final {
        using Value = std::variant<std::string, Bytes, Date, Time, Timestamp, DecimalValue>;
        std::optional<Value> value;
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

    std::int64_t GetSignedIntegerStrict(std::size_t row, std::size_t col) const;
    std::uint64_t GetUnsignedIntegerStrict(std::size_t row, std::size_t col) const;
    double GetFloatingPointStrict(std::size_t row, std::size_t col) const;
    std::string GetStringStrict(std::size_t row, std::size_t col) const;
    bool GetBoolStrict(std::size_t row, std::size_t col) const;
    Bytes GetBytesStrict(std::size_t row, std::size_t col) const;
    Date GetDateStrict(std::size_t row, std::size_t col) const;
    Time GetTimeStrict(std::size_t row, std::size_t col) const;
    Timestamp GetTimestampStrict(std::size_t row, std::size_t col) const;
    std::string GetDecimalStrict(std::size_t row, std::size_t col, std::size_t precision, std::size_t scale) const;

private:
    const Cell& GetCell(std::size_t row, std::size_t col) const;
    const Cell::Value& GetValue(std::size_t row, std::size_t col) const;

    std::vector<Column> columns_;
    std::vector<Row> rows_;
    std::size_t rows_affected_{0};
};

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
