#pragma once

#include <userver/utils/strong_typedef.hpp>

#include <userver/storages/clickhouse/io/columns/datetime64_column.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io {

using DateTime64Milli = USERVER_NAMESPACE::utils::StrongTypedef<
    columns::DateTime64ColumnMilli::Tag, std::chrono::system_clock::time_point>;
using DateTime64Micro = USERVER_NAMESPACE::utils::StrongTypedef<
    columns::DateTime64ColumnMicro::Tag, std::chrono::system_clock::time_point>;
using DateTime64Nano = USERVER_NAMESPACE::utils::StrongTypedef<
    columns::DateTime64ColumnNano::Tag, std::chrono::system_clock::time_point>;

}  // namespace storages::clickhouse::io

USERVER_NAMESPACE_END
