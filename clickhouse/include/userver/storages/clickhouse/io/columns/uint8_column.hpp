#pragma once

/// @file userver/storages/clickhouse/io/columns/uint8_column.hpp
/// @brief UInt8 column support
/// @ingroup userver_clickhouse_types

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

/// @brief Represents ClickHouse UInt8 Column
class UInt8Column final : public ClickhouseColumn<UInt8Column> {
 public:
  using cpp_type = std::uint8_t;
  using container_type = std::vector<cpp_type>;

  UInt8Column(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
