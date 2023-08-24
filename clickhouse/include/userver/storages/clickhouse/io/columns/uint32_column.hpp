#pragma once

/// @file userver/storages/clickhouse/io/columns/uint32_column.hpp
/// @brief UInt32 column support
/// @ingroup userver_clickhouse_types

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

/// @brief Represents ClickHouse UInt32 column
class UInt32Column final : public ClickhouseColumn<UInt32Column> {
 public:
  using cpp_type = std::uint32_t;
  using container_type = std::vector<cpp_type>;

  UInt32Column(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
