#include <storages/odbc/detail/result_wrapper.hpp>

#include <concepts>
#include <limits>
#include <type_traits>

#include <fmt/format.h>

#include <userver/storages/odbc/exception.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

namespace {

bool IsIntegerType(SQLSMALLINT type) noexcept {
    return type == SQL_TINYINT || type == SQL_SMALLINT || type == SQL_INTEGER || type == SQL_BIGINT;
}

bool IsFloatingPointType(SQLSMALLINT type) noexcept {
    return type == SQL_REAL || type == SQL_FLOAT || type == SQL_DOUBLE;
}

bool IsStringType(SQLSMALLINT type) noexcept {
    return type == SQL_CHAR || type == SQL_VARCHAR || type == SQL_LONGVARCHAR || type == SQL_WCHAR ||
           type == SQL_WVARCHAR || type == SQL_WLONGVARCHAR;
}

bool IsBytesType(SQLSMALLINT type) noexcept {
    return type == SQL_BINARY || type == SQL_VARBINARY || type == SQL_LONGVARBINARY;
}

bool IsDateType(SQLSMALLINT type) noexcept { return type == SQL_TYPE_DATE || type == SQL_DATE; }

bool IsTimeType(SQLSMALLINT type) noexcept { return type == SQL_TYPE_TIME || type == SQL_TIME; }

bool IsTimestampType(SQLSMALLINT type) noexcept { return type == SQL_TYPE_TIMESTAMP || type == SQL_TIMESTAMP; }

bool IsDecimalType(SQLSMALLINT type) noexcept { return type == SQL_DECIMAL || type == SQL_NUMERIC; }

[[noreturn]] void ThrowTypeMismatch(
    std::size_t row,
    std::size_t col,
    SQLSMALLINT actual_type,
    std::string_view requested_type
) {
    throw ResultSetError(fmt::format(
        "Cannot convert field at row {}, column {} with ODBC SQL type {} to {}",
        row,
        col,
        actual_type,
        requested_type
    ));
}

template <typename T>
const T& ExpectMaterializedValue(
    const ResultWrapper::Cell::Value& value,
    std::size_t row,
    std::size_t col,
    std::string_view requested_type
) {
    const auto* result = std::get_if<T>(&value);
    if (!result) {
        throw ResultSetError(fmt::format(
            "ODBC driver returned an inconsistent value representation at row {}, column {} for {}",
            row,
            col,
            requested_type
        ));
    }
    return *result;
}

template <typename T>
T ParseNumber(const std::string& value, std::size_t row, std::size_t col) {
    try {
        return utils::FromString<T>(value);
    } catch (const utils::FromStringException& ex) {
        throw ResultSetError(fmt::format(
            "Cannot convert field at row {}, column {} with value '{}' to the requested type: {}",
            row,
            col,
            value,
            ex.what()
        ));
    }
}

}  // namespace

ResultWrapper::ResultWrapper(std::vector<Column> columns, std::vector<Row> rows, std::size_t rows_affected)
    : columns_{std::move(columns)},
      rows_{std::move(rows)},
      rows_affected_{rows_affected}
{}

std::size_t ResultWrapper::RowCount() const noexcept { return rows_.size(); }

std::size_t ResultWrapper::FieldCount() const noexcept { return columns_.size(); }

std::size_t ResultWrapper::RowsAffected() const noexcept { return rows_affected_; }

const std::string& ResultWrapper::GetFieldName(std::size_t col) const {
    if (col >= columns_.size()) {
        throw FieldIndexOutOfBounds{col};
    }
    return columns_[col].name;
}

SQLSMALLINT ResultWrapper::GetColumnType(std::size_t col) const {
    if (col >= columns_.size()) {
        throw FieldIndexOutOfBounds{col};
    }
    return columns_[col].type;
}

const ResultWrapper::Cell& ResultWrapper::GetCell(std::size_t row, std::size_t col) const {
    if (row >= rows_.size()) {
        throw RowIndexOutOfBounds{row};
    }
    if (col >= columns_.size()) {
        throw FieldIndexOutOfBounds{col};
    }
    return rows_[row][col];
}

const ResultWrapper::Cell::Value& ResultWrapper::GetValue(std::size_t row, std::size_t col) const {
    const auto& cell = GetCell(row, col);
    if (!cell.value) {
        throw ResultSetError(fmt::format("Field at row {}, column {} is NULL", row, col));
    }
    return *cell.value;
}

std::string ResultWrapper::GetString(std::size_t row, std::size_t col) const {
    return std::visit(
        [](const auto& value) -> std::string {
            using Value = std::remove_cvref_t<decltype(value)>;
            if constexpr (std::same_as<Value, std::string>) {
                return value;
            } else if constexpr (std::same_as<Value, Bytes>) {
                const auto& bytes = value.GetBytes();
                if (bytes.empty()) {
                    return {};
                }
                return std::string{reinterpret_cast<const char*>(bytes.data()), bytes.size()};
            } else if constexpr (std::same_as<Value, DecimalValue>) {
                return value.representation;
            } else {
                return value.ToString();
            }
        },
        GetValue(row, col)
    );
}

std::int32_t ResultWrapper::GetInt32(std::size_t row, std::size_t col) const {
    return ParseNumber<std::int32_t>(GetString(row, col), row, col);
}

std::int64_t ResultWrapper::GetInt64(std::size_t row, std::size_t col) const {
    return ParseNumber<std::int64_t>(GetString(row, col), row, col);
}

double ResultWrapper::GetDouble(std::size_t row, std::size_t col) const {
    return ParseNumber<double>(GetString(row, col), row, col);
}

bool ResultWrapper::GetBool(std::size_t row, std::size_t col) const {
    const auto value = GetString(row, col);
    const auto ieq = utils::StrIcaseEqual{};
    if (value == "1" || ieq(value, "true") || ieq(value, "t") || ieq(value, "yes") || ieq(value, "on")) {
        return true;
    }
    if (value == "0" || ieq(value, "false") || ieq(value, "f") || ieq(value, "no") || ieq(value, "off")) {
        return false;
    }
    throw ResultSetError(
        fmt::format("Cannot convert field at row {}, column {} with value '{}' to bool", row, col, value)
    );
}

bool ResultWrapper::IsFieldNull(std::size_t row, std::size_t col) const { return !GetCell(row, col).value; }

std::int64_t ResultWrapper::GetSignedIntegerStrict(std::size_t row, std::size_t col) const {
    if (!IsIntegerType(GetColumnType(col))) {
        ThrowTypeMismatch(row, col, GetColumnType(col), "a signed integer");
    }
    return ParseNumber<
        std::int64_t>(ExpectMaterializedValue<std::string>(GetValue(row, col), row, col, "integer"), row, col);
}

std::uint64_t ResultWrapper::GetUnsignedIntegerStrict(std::size_t row, std::size_t col) const {
    if (!IsIntegerType(GetColumnType(col))) {
        ThrowTypeMismatch(row, col, GetColumnType(col), "an unsigned integer");
    }
    const auto& value = ExpectMaterializedValue<std::string>(GetValue(row, col), row, col, "integer");
    if (!value.empty() && value.front() == '-') {
        throw ResultSetError(fmt::format(
            "Cannot convert negative field at row {}, column {} with value '{}' to an unsigned integer",
            row,
            col,
            value
        ));
    }
    return ParseNumber<std::uint64_t>(value, row, col);
}

double ResultWrapper::GetFloatingPointStrict(std::size_t row, std::size_t col) const {
    if (!IsFloatingPointType(GetColumnType(col))) {
        ThrowTypeMismatch(row, col, GetColumnType(col), "a floating-point number");
    }
    return ParseNumber<
        double>(ExpectMaterializedValue<std::string>(GetValue(row, col), row, col, "floating point"), row, col);
}

std::string ResultWrapper::GetStringStrict(std::size_t row, std::size_t col) const {
    if (!IsStringType(GetColumnType(col))) {
        ThrowTypeMismatch(row, col, GetColumnType(col), "a string");
    }
    return ExpectMaterializedValue<std::string>(GetValue(row, col), row, col, "string");
}

bool ResultWrapper::GetBoolStrict(std::size_t row, std::size_t col) const {
    if (GetColumnType(col) != SQL_BIT) {
        ThrowTypeMismatch(row, col, GetColumnType(col), "bool");
    }
    return GetBool(row, col);
}

Bytes ResultWrapper::GetBytesStrict(std::size_t row, std::size_t col) const {
    if (!IsBytesType(GetColumnType(col))) {
        ThrowTypeMismatch(row, col, GetColumnType(col), "Bytes");
    }
    return ExpectMaterializedValue<Bytes>(GetValue(row, col), row, col, "Bytes");
}

Date ResultWrapper::GetDateStrict(std::size_t row, std::size_t col) const {
    if (!IsDateType(GetColumnType(col))) {
        ThrowTypeMismatch(row, col, GetColumnType(col), "Date");
    }
    return ExpectMaterializedValue<Date>(GetValue(row, col), row, col, "Date");
}

Time ResultWrapper::GetTimeStrict(std::size_t row, std::size_t col) const {
    if (!IsTimeType(GetColumnType(col))) {
        ThrowTypeMismatch(row, col, GetColumnType(col), "Time");
    }
    return ExpectMaterializedValue<Time>(GetValue(row, col), row, col, "Time");
}

Timestamp ResultWrapper::GetTimestampStrict(std::size_t row, std::size_t col) const {
    if (!IsTimestampType(GetColumnType(col))) {
        ThrowTypeMismatch(row, col, GetColumnType(col), "Timestamp");
    }
    return ExpectMaterializedValue<Timestamp>(GetValue(row, col), row, col, "Timestamp");
}

std::string ResultWrapper::GetDecimalStrict(std::size_t row, std::size_t col, std::size_t precision, std::size_t scale)
    const {
    if (!IsDecimalType(GetColumnType(col))) {
        ThrowTypeMismatch(row, col, GetColumnType(col), "Decimal");
    }
    const auto& value = ExpectMaterializedValue<DecimalValue>(GetValue(row, col), row, col, "Decimal");
    if (value.precision != precision || value.scale != scale) {
        throw ResultSetError(fmt::format(
            "Cannot convert Decimal field at row {}, column {} with precision {} and scale {} to Decimal<{}, {}>",
            row,
            col,
            value.precision,
            value.scale,
            precision,
            scale
        ));
    }
    return value.representation;
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
