#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <variant>
#include <vector>

#include <sql.h>

#include <userver/storages/odbc/bulk.hpp>
#include <userver/storages/odbc/impl/parameter.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

inline constexpr std::size_t kMaxNativeBulkRows = 65'536;
inline constexpr std::size_t kMaxBulkBindingBytes = 64 * 1024 * 1024;

enum class BulkColumnType {
    kBoolean,
    kInteger,
    kFloatingPoint,
    kString,
    kBytes,
    kDate,
    kTime,
    kTimestamp,
    kDecimal,
};

struct BulkColumnDescription final {  // NOLINT(cppcoreguidelines-pro-type-member-init)
    BulkColumnType type;
    std::size_t max_payload_size{0};
    std::uint8_t decimal_precision{0};
    std::uint8_t decimal_scale{0};
    bool timestamp_has_fraction{false};
};

struct BulkLayout final {
    std::size_t rows_count{0};
    bool native_binding_allowed{true};
    std::vector<BulkColumnDescription> columns;
};

template <typename T>
struct BulkValues final {
    std::vector<T> values;
};

struct BulkInlineValues final {
    std::vector<SQLCHAR> values;
    std::size_t stride{0};
};

using BulkColumnValues = std::variant<
    BulkValues<SQLCHAR>,
    BulkValues<SQLBIGINT>,
    BulkValues<SQLDOUBLE>,
    BulkInlineValues,
    BulkValues<SQL_DATE_STRUCT>,
    BulkValues<SQL_TIME_STRUCT>,
    BulkValues<SQL_TIMESTAMP_STRUCT>,
    BulkValues<SQL_NUMERIC_STRUCT>>;

struct BulkColumnBinding final {
    SQLSMALLINT c_type{0};
    SQLSMALLINT sql_type{0};
    SQLULEN column_size{0};
    SQLSMALLINT decimal_digits{0};
    SQLLEN buffer_size{0};
    BulkColumnValues values;
    std::vector<SQLLEN> indicators;

    SQLPOINTER Data() noexcept;
};

inline constexpr SQLUSMALLINT kBulkStatusUntouched = std::numeric_limits<SQLUSMALLINT>::max();
inline constexpr SQLULEN kBulkProcessedUntouched = std::numeric_limits<SQLULEN>::max();

struct BulkBindings final {
    std::size_t rows_count{0};
    std::size_t storage_bytes{0};
    std::vector<BulkColumnBinding> columns;
    std::vector<SQLUSMALLINT> statuses;
    SQLULEN processed{kBulkProcessedUntouched};
};

struct ParsedBulkStatuses final {
    std::vector<BulkRowStatus> statuses;
    bool has_unrecognized_processed_status{false};
};

struct BulkChunkStatusDecision final {
    std::vector<BulkRowStatus> statuses;
    std::optional<std::size_t> processed;
    bool is_failure{false};
};

/// Validates every row before a connection is acquired or any row is executed.
BulkLayout ValidateBulkRows(const impl::ParameterRows& rows);

/// Computes the peak owned binding storage for synthetic per-column slot sizes.
/// Returns null when the 64 MiB native-binding budget would be exceeded.
std::optional<std::size_t> TryEstimateBulkBindingStorageBytes(
    std::size_t rows_count,
    std::span<const std::size_t> value_slot_sizes
);

/// Transposes one validated row range to owned column-wise ODBC buffers. A
/// null result means that the 64 MiB native-binding budget was exceeded. The
/// caller must reduce a multi-row range; only a one-row range may fall back to
/// scalar execution.
std::optional<BulkBindings> TryBuildBulkBindings(
    const impl::ParameterRows& rows,
    const BulkLayout& layout,
    std::size_t begin,
    std::size_t count
);

/// Converts a driver parameter-status array without interpreting untrusted
/// status values. `global_success` is true only for SQL_SUCCESS.
ParsedBulkStatuses ParseBulkRowStatuses(
    std::span<const SQLUSMALLINT> raw_statuses,
    std::size_t requested,
    std::optional<std::size_t> processed,
    bool statuses_are_trusted,
    bool global_success
);

/// Applies the PARC trust and SQLExecute return-code policy for one chunk.
BulkChunkStatusDecision EvaluateBulkChunkStatuses(
    std::span<const SQLUSMALLINT> raw_statuses,
    std::size_t requested,
    std::optional<std::size_t> processed,
    bool statuses_are_trusted,
    SQLRETURN execute_result
);

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
