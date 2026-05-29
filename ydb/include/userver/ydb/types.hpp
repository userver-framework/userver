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
    std::optional<Timestamp>>;

struct InsertColumn {
    std::string name;
    InsertColumnValue value;
};

using InsertRow = std::vector<InsertColumn>;

}  // namespace ydb

USERVER_NAMESPACE_END
