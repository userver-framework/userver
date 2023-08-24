#pragma once

/// @file userver/storages/clickhouse/io/columns/datetime_column.hpp
/// @brief DateTime column support
/// @ingroup userver_clickhouse_types

#include <chrono>

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

/// @brief Represents ClickHouse DateTime column
class DateTimeColumn final : public ClickhouseColumn<DateTimeColumn> {
 public:
  using cpp_type = std::chrono::system_clock::time_point;
  using container_type = std::vector<cpp_type>;

  DateTimeColumn(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
