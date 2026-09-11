#pragma once

/// @file userver/ydb/types.hpp
/// @brief YDB primitive types and insert row column types

#include <chrono>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <userver/formats/json_fwd.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

/*
 * YDB type      | C++ type
 * ------------------------------
 * Bool          | bool
 * Int8          | std::int8_t
 * Uint8         | std::uint8_t
 * Int16         | std::int16_t
 * Uint16        | std::uint16_t
 * Int32         | std::int32_t
 * Uint32        | std::uint32_t
 * Int64         | std::int64_t
 * Uint64        | std::uint64_t
 * Float         | N/A
 * Double        | double
 * Decimal       | ydb::Decimal
 * Date          | N/A
 * Datetime      | N/A
 * Timestamp     | std::chrono::system_clock::time_point
 * Interval      | N/A
 * TzDate        | N/A
 * TzDatetime    | N/A
 * TzTimestamp   | N/A
 * String        | std::string
 * Utf8          | ydb::Utf8
 * Yson          | N/A
 * Json          | formats::json::Value
 * Uuid          | N/A
 * JsonDocument  | ydb::JsonDocument
 * DyNumber      | N/A
 *
 */

using Timestamp = std::chrono::system_clock::time_point;

class Utf8Tag {};
using Utf8 = utils::StrongTypedef<Utf8Tag, std::string>;

class JsonDocumentTag {};
using JsonDocument = utils::StrongTypedef<JsonDocumentTag, formats::json::Value>;

// A YDB Decimal value as a (value, precision, scale) triple.
//
// In YDB, `precision` and `scale` are part of the column type, not of the
// value, and must be provided when writing. `ydb::Decimal` is a thin transport
// wrapper around `NYdb::TDecimalValue` that preserves them as-is; it is NOT
// an arithmetic type. For numeric operations, parsing from numbers, rounding
// etc. use `decimal64::Decimal<Prec, RoundPolicy>` (see
// `userver/ydb/io/decimal64.hpp`), whose `ValueTraits` are also provided by
// this library.
//
// `value` stores the decimal in the same string form that the YDB SDK
// produces and accepts (e.g. `"123.456789"`). YDB returns decimals in a
// canonical form with trailing zeros stripped, so a value written as
// `"7.500000000"` will be read back as `"7.5"`. Consequently `operator==`
// performs a strict string comparison and is NOT a numeric equality check.
//
// The default `precision = 22`, `scale = 9` matches the most common
// "money-like" YDB schema `Decimal(22, 9)`. They are exposed as
// `kDefaultPrecision` / `kDefaultScale` for convenience but should be
// overridden whenever the column uses a different type, e.g.
// `ydb::Decimal{"123.456789012345678", 35, 18}`.
struct Decimal {
    static constexpr std::uint8_t kDefaultPrecision = 22;
    static constexpr std::uint8_t kDefaultScale = 9;

    std::string value;
    std::uint8_t precision{kDefaultPrecision};
    std::uint8_t scale{kDefaultScale};

    Decimal() = default;

    Decimal(std::string value, std::uint8_t precision, std::uint8_t scale)
        : value(std::move(value)), precision(precision), scale(scale) {}

    friend bool operator==(const Decimal& lhs, const Decimal& rhs) {
        return lhs.precision == rhs.precision && lhs.scale == rhs.scale && lhs.value == rhs.value;
    }

    friend bool operator!=(const Decimal& lhs, const Decimal& rhs) { return !(lhs == rhs); }
};

using InsertColumnValue = std::variant<
    std::string,
    bool,
    std::int8_t,
    std::uint8_t,
    std::int16_t,
    std::uint16_t,
    std::int32_t,
    std::uint32_t,
    std::int64_t,
    std::uint64_t,
    double,
    Utf8,
    Timestamp,
    std::optional<std::string>,
    std::optional<bool>,
    std::optional<std::int8_t>,
    std::optional<std::uint8_t>,
    std::optional<std::int16_t>,
    std::optional<std::uint16_t>,
    std::optional<std::int32_t>,
    std::optional<std::uint32_t>,
    std::optional<std::int64_t>,
    std::optional<std::uint64_t>,
    std::optional<double>,
    std::optional<Utf8>,
    std::optional<Timestamp>,
    Decimal>;

struct InsertColumn {
    std::string name;
    InsertColumnValue value;
};

using InsertRow = std::vector<InsertColumn>;

}  // namespace ydb

USERVER_NAMESPACE_END
