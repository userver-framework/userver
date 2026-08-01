#include <storages/odbc/detail/result_wrapper.hpp>

#include <fmt/format.h>

#include <userver/storages/odbc/exception.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

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

const std::string& ResultWrapper::GetValue(std::size_t row, std::size_t col) const {
    const auto& cell = GetCell(row, col);
    if (!cell.value) {
        throw ResultSetError(fmt::format("Field at row {}, column {} is NULL", row, col));
    }
    return *cell.value;
}

std::string ResultWrapper::GetString(std::size_t row, std::size_t col) const { return GetValue(row, col); }

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

std::int32_t ResultWrapper::GetInt32(std::size_t row, std::size_t col) const {
    return ParseNumber<std::int32_t>(GetValue(row, col), row, col);
}

std::int64_t ResultWrapper::GetInt64(std::size_t row, std::size_t col) const {
    return ParseNumber<std::int64_t>(GetValue(row, col), row, col);
}

double ResultWrapper::GetDouble(std::size_t row, std::size_t col) const {
    return ParseNumber<double>(GetValue(row, col), row, col);
}

bool ResultWrapper::GetBool(std::size_t row, std::size_t col) const {
    const auto& value = GetValue(row, col);
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

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
