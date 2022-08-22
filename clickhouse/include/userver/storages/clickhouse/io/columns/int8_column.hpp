#pragma once

/// @file userver/storages/clickhouse/io/columns/int8_column.hpp
/// @brief Int8 column support
/// @ingroup userver_clickhouse_types

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

/// @brief Represents ClickHouse UInt8 Column
class Int8Column final : public ClickhouseColumn<Int8Column> {
 public:
  using cpp_type = std::int8_t;
  using container_type = std::vector<cpp_type>;

  Int8Column(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
