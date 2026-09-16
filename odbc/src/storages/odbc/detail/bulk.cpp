#include <storages/odbc/detail/bulk.hpp>

#include <algorithm>
#include <cstring>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

#include <fmt/format.h>
#include <sqlext.h>

#include <userver/storages/odbc/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

namespace {

BulkColumnType NormalizeType(impl::ParameterType type, std::size_t row, std::size_t column) {
    using impl::ParameterType;
    switch (type) {
        case ParameterType::kBoolean:
            return BulkColumnType::kBoolean;
        case ParameterType::kSignedInteger:
        case ParameterType::kUnsignedInteger:
            return BulkColumnType::kInteger;
        case ParameterType::kFloatingPoint:
            return BulkColumnType::kFloatingPoint;
        case ParameterType::kString:
            return BulkColumnType::kString;
        case ParameterType::kBytes:
            return BulkColumnType::kBytes;
        case ParameterType::kDate:
            return BulkColumnType::kDate;
        case ParameterType::kTime:
            return BulkColumnType::kTime;
        case ParameterType::kTimestamp:
            return BulkColumnType::kTimestamp;
        case ParameterType::kDecimal:
            return BulkColumnType::kDecimal;
        case ParameterType::kUnknown:
            throw LogicError(fmt::format(
                "ODBC bulk parameter at row {}, column {} is an untyped NULL; use a typed std::optional",
                row,
                column
            ));
    }
    throw LogicError("Unknown ODBC bulk parameter type");
}

std::size_t PayloadSize(const impl::Parameter& parameter) {
    switch (parameter.GetType()) {
        case impl::ParameterType::kString:
            return parameter.Get<std::string>().size();
        case impl::ParameterType::kBytes:
            return parameter.Get<Bytes>().GetBytes().size();
        default:
            return 0;
    }
}

void ValidateRepresentablePayload(std::size_t size, std::size_t row, std::size_t column) {
    if (size > static_cast<std::size_t>(std::numeric_limits<SQLLEN>::max())) {
        throw LogicError(fmt::format(
            "ODBC bulk parameter at row {}, column {} has {} bytes, which does not fit SQLLEN",
            row,
            column,
            size
        ));
    }
}

std::size_t CheckedMultiply(std::size_t left, std::size_t right, std::string_view what) {
    if (left != 0 && right > std::numeric_limits<std::size_t>::max() / left) {
        throw LogicError(fmt::format("ODBC bulk {} size overflows size_t", what));
    }
    return left * right;
}

std::size_t CheckedAdd(std::size_t left, std::size_t right, std::string_view what) {
    if (right > std::numeric_limits<std::size_t>::max() - left) {
        throw LogicError(fmt::format("ODBC bulk {} size overflows size_t", what));
    }
    return left + right;
}

template <typename T>
void ValidateVectorCount(std::size_t count, std::string_view what) {
    if (count > std::vector<T>{}.max_size()) {
        throw LogicError(fmt::format("ODBC bulk {} cannot be represented by a vector", what));
    }
}

SQL_NUMERIC_STRUCT MakeNumericStruct(const impl::DecimalParameter& parameter) {
    SQL_NUMERIC_STRUCT result{};
    result.precision = parameter.precision;
    result.scale = static_cast<SQLSCHAR>(parameter.scale);
    result.sign = parameter.representation.front() == '-' ? 0 : 1;

    for (const char ch : parameter.representation) {
        if (ch == '-' || ch == '+' || ch == '.') {
            continue;
        }
        auto carry = static_cast<unsigned>(ch - '0');
        for (auto& byte : result.val) {
            const auto value = static_cast<unsigned>(byte) * 10U + carry;
            byte = static_cast<SQLCHAR>(value & 0xffU);
            carry = value >> 8U;
        }
        if (carry != 0) {
            throw LogicError("ODBC bulk Decimal magnitude exceeds SQL_NUMERIC_STRUCT capacity");
        }
    }
    return result;
}

std::size_t FixedValueSize(BulkColumnType type) {
    switch (type) {
        case BulkColumnType::kBoolean:
            return sizeof(SQLCHAR);
        case BulkColumnType::kInteger:
            return sizeof(SQLBIGINT);
        case BulkColumnType::kFloatingPoint:
            return sizeof(SQLDOUBLE);
        case BulkColumnType::kDate:
            return sizeof(SQL_DATE_STRUCT);
        case BulkColumnType::kTime:
            return sizeof(SQL_TIME_STRUCT);
        case BulkColumnType::kTimestamp:
            return sizeof(SQL_TIMESTAMP_STRUCT);
        case BulkColumnType::kDecimal:
            return sizeof(SQL_NUMERIC_STRUCT);
        case BulkColumnType::kString:
        case BulkColumnType::kBytes:
            return 0;
    }
    throw LogicError("Unknown ODBC bulk column type");
}

std::size_t ChunkPayloadStride(
    const impl::ParameterRows& rows,
    std::size_t begin,
    std::size_t count,
    std::size_t column
) {
    std::size_t stride = 1;
    for (std::size_t row = begin; row < begin + count; ++row) {
        stride = std::max(stride, PayloadSize(rows[row][column]));
    }
    return stride;
}

BulkColumnBinding MakeBinding(BulkColumnDescription description, std::size_t count, std::size_t stride) {
    BulkColumnBinding binding;
    binding.indicators.resize(count);
    switch (description.type) {
        case BulkColumnType::kBoolean:
            binding.c_type = SQL_C_BIT;
            binding.sql_type = SQL_BIT;
            binding.column_size = 1;
            binding.buffer_size = sizeof(SQLCHAR);
            binding.values = BulkValues<SQLCHAR>{std::vector<SQLCHAR>(count)};
            break;
        case BulkColumnType::kInteger:
            binding.c_type = SQL_C_SBIGINT;
            binding.sql_type = SQL_BIGINT;
            binding.column_size = 19;
            binding.buffer_size = sizeof(SQLBIGINT);
            binding.values = BulkValues<SQLBIGINT>{std::vector<SQLBIGINT>(count)};
            break;
        case BulkColumnType::kFloatingPoint:
            binding.c_type = SQL_C_DOUBLE;
            binding.sql_type = SQL_DOUBLE;
            binding.column_size = 15;
            binding.buffer_size = sizeof(SQLDOUBLE);
            binding.values = BulkValues<SQLDOUBLE>{std::vector<SQLDOUBLE>(count)};
            break;
        case BulkColumnType::kString:
            binding.c_type = SQL_C_CHAR;
            binding.sql_type = SQL_VARCHAR;
            binding.column_size = static_cast<SQLULEN>(stride);
            binding.buffer_size = static_cast<SQLLEN>(stride);
            binding.values =
                BulkInlineValues{std::vector<SQLCHAR>(CheckedMultiply(count, stride, "string buffer")), stride};
            break;
        case BulkColumnType::kBytes:
            binding.c_type = SQL_C_BINARY;
            binding.sql_type = SQL_LONGVARBINARY;
            binding.column_size = static_cast<SQLULEN>(stride);
            binding.buffer_size = static_cast<SQLLEN>(stride);
            binding
                .values = BulkInlineValues{std::vector<SQLCHAR>(CheckedMultiply(count, stride, "byte buffer")), stride};
            break;
        case BulkColumnType::kDate:
            binding.c_type = SQL_C_TYPE_DATE;
            binding.sql_type = SQL_TYPE_DATE;
            binding.column_size = 10;
            binding.buffer_size = sizeof(SQL_DATE_STRUCT);
            binding.values = BulkValues<SQL_DATE_STRUCT>{std::vector<SQL_DATE_STRUCT>(count)};
            break;
        case BulkColumnType::kTime:
            binding.c_type = SQL_C_TYPE_TIME;
            binding.sql_type = SQL_TYPE_TIME;
            binding.column_size = 8;
            binding.buffer_size = sizeof(SQL_TIME_STRUCT);
            binding.values = BulkValues<SQL_TIME_STRUCT>{std::vector<SQL_TIME_STRUCT>(count)};
            break;
        case BulkColumnType::kTimestamp:
            binding.c_type = SQL_C_TYPE_TIMESTAMP;
            binding.sql_type = SQL_TYPE_TIMESTAMP;
            binding.column_size = description.timestamp_has_fraction ? 29 : 19;
            binding.decimal_digits = description.timestamp_has_fraction ? 9 : 0;
            binding.buffer_size = sizeof(SQL_TIMESTAMP_STRUCT);
            binding.values = BulkValues<SQL_TIMESTAMP_STRUCT>{std::vector<SQL_TIMESTAMP_STRUCT>(count)};
            break;
        case BulkColumnType::kDecimal:
            binding.c_type = SQL_C_NUMERIC;
            binding.sql_type = SQL_DECIMAL;
            binding.column_size = description.decimal_precision;
            binding.decimal_digits = description.decimal_scale;
            binding.buffer_size = 0;
            binding.values = BulkValues<SQL_NUMERIC_STRUCT>{std::vector<SQL_NUMERIC_STRUCT>(count)};
            break;
    }
    return binding;
}

void CopyInlinePayload(BulkInlineValues& target, std::size_t row, const void* source, std::size_t size) {
    if (size == 0) {
        return;
    }
    std::memcpy(target.values.data() + row * target.stride, source, size);
}

void StoreValue(BulkColumnBinding& binding, const impl::Parameter& parameter, std::size_t row) {
    if (parameter.IsNull()) {
        binding.indicators[row] = SQL_NULL_DATA;
        return;
    }

    using impl::ParameterType;
    switch (parameter.GetType()) {
        case ParameterType::kBoolean:
            std::get<BulkValues<SQLCHAR>>(binding.values).values[row] = parameter.Get<bool>() ? 1 : 0;
            break;
        case ParameterType::kSignedInteger:
            std::get<BulkValues<SQLBIGINT>>(binding.values).values[row] = parameter.Get<std::int64_t>();
            break;
        case ParameterType::kUnsignedInteger:
            std::get<BulkValues<SQLBIGINT>>(binding.values)
                .values[row] = static_cast<SQLBIGINT>(parameter.Get<std::uint64_t>());
            break;
        case ParameterType::kFloatingPoint:
            std::get<BulkValues<SQLDOUBLE>>(binding.values).values[row] = parameter.Get<double>();
            break;
        case ParameterType::kString: {
            const auto& value = parameter.Get<std::string>();
            auto& target = std::get<BulkInlineValues>(binding.values);
            CopyInlinePayload(target, row, value.data(), value.size());
            binding.indicators[row] = static_cast<SQLLEN>(value.size());
            return;
        }
        case ParameterType::kBytes: {
            const auto& value = parameter.Get<Bytes>().GetBytes();
            auto& target = std::get<BulkInlineValues>(binding.values);
            CopyInlinePayload(target, row, value.data(), value.size());
            binding.indicators[row] = static_cast<SQLLEN>(value.size());
            return;
        }
        case ParameterType::kDate: {
            const auto& value = parameter.Get<Date>();
            std::get<BulkValues<SQL_DATE_STRUCT>>(binding.values).values[row] = SQL_DATE_STRUCT{
                static_cast<SQLSMALLINT>(value.GetYear()),
                static_cast<SQLUSMALLINT>(value.GetMonth()),
                static_cast<SQLUSMALLINT>(value.GetDay()),
            };
            break;
        }
        case ParameterType::kTime: {
            const auto& value = parameter.Get<Time>();
            std::get<BulkValues<SQL_TIME_STRUCT>>(binding.values).values[row] = SQL_TIME_STRUCT{
                static_cast<SQLUSMALLINT>(value.GetHour()),
                static_cast<SQLUSMALLINT>(value.GetMinute()),
                static_cast<SQLUSMALLINT>(value.GetSecond()),
            };
            break;
        }
        case ParameterType::kTimestamp: {
            const auto& value = parameter.Get<Timestamp>();
            const auto& date = value.GetDate();
            const auto& time = value.GetTime();
            std::get<BulkValues<SQL_TIMESTAMP_STRUCT>>(binding.values).values[row] = SQL_TIMESTAMP_STRUCT{
                static_cast<SQLSMALLINT>(date.GetYear()),
                static_cast<SQLUSMALLINT>(date.GetMonth()),
                static_cast<SQLUSMALLINT>(date.GetDay()),
                static_cast<SQLUSMALLINT>(time.GetHour()),
                static_cast<SQLUSMALLINT>(time.GetMinute()),
                static_cast<SQLUSMALLINT>(time.GetSecond()),
                static_cast<SQLUINTEGER>(value.GetFractionNanoseconds()),
            };
            break;
        }
        case ParameterType::kDecimal:
            std::get<BulkValues<SQL_NUMERIC_STRUCT>>(binding.values)
                .values[row] = MakeNumericStruct(parameter.Get<impl::DecimalParameter>());
            break;
        case ParameterType::kUnknown:
            throw LogicError("Untyped NULL reached ODBC bulk binding after validation");
    }
    binding.indicators
        [row] = binding.buffer_size == 0 ? static_cast<SQLLEN>(sizeof(SQL_NUMERIC_STRUCT)) : binding.buffer_size;
}

BulkRowStatus ParseStatus(SQLUSMALLINT status) {
    switch (status) {
        case SQL_PARAM_SUCCESS:
            return BulkRowStatus::kSuccess;
        case SQL_PARAM_SUCCESS_WITH_INFO:
            return BulkRowStatus::kSuccessWithInfo;
        case SQL_PARAM_ERROR:
            return BulkRowStatus::kError;
        case SQL_PARAM_UNUSED:
            return BulkRowStatus::kUnused;
        case SQL_PARAM_DIAG_UNAVAILABLE:
            return BulkRowStatus::kDiagnosticsUnavailable;
        default:
            return BulkRowStatus::kUnknown;
    }
}

}  // namespace

SQLPOINTER BulkColumnBinding::Data() noexcept {
    return std::visit([](auto& value) -> SQLPOINTER { return value.values.data(); }, values);
}

BulkLayout ValidateBulkRows(const impl::ParameterRows& rows) {
    BulkLayout layout;
    layout.rows_count = rows.size();
    if (rows.empty()) {
        return layout;
    }

    const auto columns_count = rows.front().size();
    if (columns_count == 0) {
        throw LogicError("ODBC bulk parameter rows must contain at least one column");
    }
    if (columns_count > static_cast<std::size_t>(std::numeric_limits<SQLSMALLINT>::max())) {
        throw LogicError("ODBC bulk parameter column count does not fit the ODBC SQLSMALLINT parameter index");
    }
    const auto layout_bytes = CheckedMultiply(columns_count, sizeof(BulkColumnDescription), "layout metadata");
    layout.native_binding_allowed = layout_bytes <= kMaxBulkBindingBytes;
    if (layout.native_binding_allowed) {
        layout.columns.reserve(columns_count);
    }
    for (std::size_t column = 0; layout.native_binding_allowed && column < columns_count; ++column) {
        const auto& parameter = rows.front()[column];
        BulkColumnDescription description{.type = NormalizeType(parameter.GetType(), 0, column)};
        if (description.type == BulkColumnType::kDecimal) {
            const auto& decimal = parameter.Get<impl::DecimalParameter>();
            description.decimal_precision = decimal.precision;
            description.decimal_scale = decimal.scale;
        }
        layout.columns.push_back(description);
    }

    for (std::size_t row = 0; row < rows.size(); ++row) {
        if (rows[row].size() != columns_count) {
            throw LogicError(fmt::format(
                "ODBC bulk parameter row {} has {} columns, expected {}",
                row,
                rows[row].size(),
                columns_count
            ));
        }
        for (std::size_t column = 0; column < columns_count; ++column) {
            const auto& parameter = rows[row][column];
            auto description =
                layout.native_binding_allowed
                    ? layout.columns[column]
                    : BulkColumnDescription{.type = NormalizeType(rows.front()[column].GetType(), 0, column)};
            if (description.type == BulkColumnType::kDecimal) {
                const auto& decimal = rows.front()[column].Get<impl::DecimalParameter>();
                description.decimal_precision = decimal.precision;
                description.decimal_scale = decimal.scale;
            }
            const auto normalized_type = NormalizeType(parameter.GetType(), row, column);
            if (normalized_type != description.type) {
                throw LogicError(
                    fmt::format("ODBC bulk parameter column {} mixes incompatible types at row {}", column, row)
                );
            }
            if (!parameter.IsNull() && parameter.GetType() == impl::ParameterType::kUnsignedInteger &&
                parameter.Get<std::uint64_t>() > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
            {
                throw LogicError(fmt::format(
                    "ODBC unsigned bulk parameter at row {}, column {} is outside the portable SQL BIGINT range",
                    row,
                    column
                ));
            }
            if (description.type == BulkColumnType::kDecimal) {
                const auto& decimal = parameter.Get<impl::DecimalParameter>();
                if (decimal.precision != description.decimal_precision || decimal.scale != description.decimal_scale) {
                    throw LogicError(fmt::format(
                        "ODBC Decimal bulk parameter column {} mixes precision/scale at row {}",
                        column,
                        row
                    ));
                }
            }
            if (description.type == BulkColumnType::kTimestamp &&
                parameter.Get<Timestamp>().GetFractionNanoseconds() != 0)
            {
                description.timestamp_has_fraction = true;
            }
            const auto payload_size = PayloadSize(parameter);
            ValidateRepresentablePayload(payload_size, row, column);
            description.max_payload_size = std::max(description.max_payload_size, payload_size);
            if (layout.native_binding_allowed) {
                layout.columns[column] = description;
            }
        }
    }
    return layout;
}

std::optional<BulkBindings> TryBuildBulkBindings(
    const impl::ParameterRows& rows,
    const BulkLayout& layout,
    std::size_t begin,
    std::size_t count
) {
    if (count == 0 || begin > rows.size() || count > rows.size() - begin || layout.rows_count != rows.size()) {
        throw LogicError("Invalid ODBC bulk binding row range");
    }
    if (!layout.native_binding_allowed) {
        return std::nullopt;
    }
    if (count > kMaxNativeBulkRows) {
        throw LogicError(fmt::format("ODBC native bulk chunk has {} rows, maximum is {}", count, kMaxNativeBulkRows));
    }
    if (layout.columns.size() != rows.front().size()) {
        throw LogicError("Invalid ODBC bulk layout column count");
    }
    if (count > static_cast<std::size_t>(std::numeric_limits<SQLULEN>::max())) {
        throw LogicError("ODBC bulk row count does not fit SQLULEN");
    }
    ValidateVectorCount<SQLUSMALLINT>(count, "status array");
    ValidateVectorCount<SQLLEN>(count, "indicator array");

    const auto metadata_bytes = CheckedMultiply(layout.columns.size(), sizeof(BulkColumnBinding), "column metadata");
    std::size_t storage_bytes =
        CheckedAdd(CheckedMultiply(count, sizeof(SQLUSMALLINT), "status array"), metadata_bytes, "binding storage");
    if (storage_bytes > kMaxBulkBindingBytes) {
        return std::nullopt;
    }
    for (std::size_t column = 0; column < layout.columns.size(); ++column) {
        const auto type = layout.columns[column].type;
        const auto fixed_size = FixedValueSize(type);
        if (fixed_size == 0) {
            const auto stride = ChunkPayloadStride(rows, begin, count, column);
            if (stride >
                std::min(
                    static_cast<std::size_t>(std::numeric_limits<SQLULEN>::max()),
                    static_cast<std::size_t>(std::numeric_limits<SQLLEN>::max())
                ))
            {
                throw LogicError("ODBC bulk variable-width slot does not fit SQLULEN/SQLLEN");
            }
            ValidateVectorCount<SQLCHAR>(CheckedMultiply(count, stride, "variable-width buffer"), "buffer");
            storage_bytes =
                CheckedAdd(storage_bytes, CheckedMultiply(count, stride, "variable-width buffer"), "binding storage");
        } else {
            storage_bytes =
                CheckedAdd(storage_bytes, CheckedMultiply(count, fixed_size, "value buffer"), "binding storage");
        }
        storage_bytes =
            CheckedAdd(storage_bytes, CheckedMultiply(count, sizeof(SQLLEN), "indicator array"), "binding storage");
        if (storage_bytes > kMaxBulkBindingBytes) {
            return std::nullopt;
        }
    }

    BulkBindings bindings;
    bindings.rows_count = count;
    bindings.storage_bytes = storage_bytes;
    bindings.statuses.assign(count, kBulkStatusUntouched);
    bindings.columns.reserve(layout.columns.size());
    for (std::size_t column = 0; column < layout.columns.size(); ++column) {
        const auto fixed_size = FixedValueSize(layout.columns[column].type);
        const auto stride = fixed_size == 0 ? ChunkPayloadStride(rows, begin, count, column) : fixed_size;
        auto binding = MakeBinding(layout.columns[column], count, stride);
        for (std::size_t row = 0; row < count; ++row) {
            StoreValue(binding, rows[begin + row][column], row);
        }
        bindings.columns.push_back(std::move(binding));
    }
    return bindings;
}

std::optional<std::size_t> TryEstimateBulkBindingStorageBytes(
    std::size_t rows_count,
    std::span<const std::size_t> value_slot_sizes
) {
    const auto metadata_bytes = CheckedMultiply(value_slot_sizes.size(), sizeof(BulkColumnBinding), "column metadata");
    auto storage_bytes = CheckedAdd(
        CheckedMultiply(rows_count, sizeof(SQLUSMALLINT), "status array"),
        metadata_bytes,
        "binding storage"
    );
    for (const auto slot_size : value_slot_sizes) {
        storage_bytes = CheckedAdd(
            storage_bytes,
            CheckedMultiply(rows_count, sizeof(SQLLEN), "indicator array"),
            "binding storage"
        );
        storage_bytes =
            CheckedAdd(storage_bytes, CheckedMultiply(rows_count, slot_size, "value buffer"), "binding storage");
        if (storage_bytes > kMaxBulkBindingBytes) {
            return std::nullopt;
        }
    }
    if (storage_bytes > kMaxBulkBindingBytes) {
        return std::nullopt;
    }
    return storage_bytes;
}

ParsedBulkStatuses ParseBulkRowStatuses(
    std::span<const SQLUSMALLINT> raw_statuses,
    std::size_t requested,
    std::optional<std::size_t> processed,
    bool statuses_are_trusted,
    bool global_success
) {
    if (raw_statuses.size() != requested) {
        throw LogicError("ODBC bulk status array size does not match the requested row count");
    }
    if (processed && *processed > requested) {
        throw LogicError("ODBC bulk processed row count exceeds the requested row count");
    }

    ParsedBulkStatuses result;
    result.statuses.assign(requested, BulkRowStatus::kUnknown);
    if (!statuses_are_trusted) {
        if (global_success && processed == requested) {
            std::fill(result.statuses.begin(), result.statuses.end(), BulkRowStatus::kSuccess);
        }
        return result;
    }

    for (std::size_t row = 0; row < requested; ++row) {
        result.statuses[row] = ParseStatus(raw_statuses[row]);
    }
    if (processed) {
        for (std::size_t row = 0; row < *processed; ++row) {
            if (result.statuses[row] == BulkRowStatus::kUnknown) {
                result.has_unrecognized_processed_status = true;
            }
        }
    }
    return result;
}

BulkChunkStatusDecision EvaluateBulkChunkStatuses(
    std::span<const SQLUSMALLINT> raw_statuses,
    std::size_t requested,
    std::optional<std::size_t> processed,
    bool statuses_are_trusted,
    SQLRETURN execute_result
) {
    auto parsed = ParseBulkRowStatuses(
        raw_statuses,
        requested,
        processed,
        statuses_are_trusted,
        execute_result == SQL_SUCCESS || execute_result == SQL_NO_DATA
    );
    if (execute_result == SQL_NO_DATA && processed == requested) {
        std::fill(parsed.statuses.begin(), parsed.statuses.end(), BulkRowStatus::kSuccess);
        parsed.has_unrecognized_processed_status = false;
    }

    BulkChunkStatusDecision decision{
        .statuses = std::move(parsed.statuses),
        .processed = SQL_SUCCEEDED(execute_result) || execute_result == SQL_NO_DATA ? processed : std::nullopt,
        .is_failure = !SQL_SUCCEEDED(execute_result) && execute_result != SQL_NO_DATA,
    };
    if (decision.is_failure) {
        return decision;
    }
    decision.is_failure = processed && *processed != requested;
    if (!statuses_are_trusted) {
        return decision;
    }

    decision.is_failure =
        decision.is_failure || parsed.has_unrecognized_processed_status ||
        std::find(decision.statuses.begin(), decision.statuses.end(), BulkRowStatus::kError) != decision.statuses.end();
    const auto processed_prefix = processed.value_or(0);
    for (std::size_t row = 0; row < processed_prefix; ++row) {
        decision.is_failure =
            decision.is_failure || decision.statuses[row] == BulkRowStatus::kUnused ||
            decision.statuses[row] == BulkRowStatus::kUnknown;
    }
    return decision;
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
