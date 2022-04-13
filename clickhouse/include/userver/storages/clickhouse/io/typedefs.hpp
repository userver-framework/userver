#pragma once

/// @file userver/storages/clickhouse/io/typedefs.hpp

#include <userver/utils/strong_typedef.hpp>

#include <userver/storages/clickhouse/io/columns/datetime64_column.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io {

/// @brief StrongTypedef for serializing `system_clock::time_point`
/// to DateTime64(3) format when used as a query argument
using DateTime64Milli = USERVER_NAMESPACE::utils::StrongTypedef<
    columns::DateTime64ColumnMilli::Tag, std::chrono::system_clock::time_point>;

/// @brief StrongTypedef for serializing `system_clock::time_point`
/// to DateTime64(6) format when used as a query argument
using DateTime64Micro = USERVER_NAMESPACE::utils::StrongTypedef<
    columns::DateTime64ColumnMicro::Tag, std::chrono::system_clock::time_point>;

/// @brief StrongTypedef for serializing `system_clock::time_point`
/// to DateTime64(9) format when used as a query argument
using DateTime64Nano = USERVER_NAMESPACE::utils::StrongTypedef<
    columns::DateTime64ColumnNano::Tag, std::chrono::system_clock::time_point>;

}  // namespace storages::clickhouse::io

USERVER_NAMESPACE_END
