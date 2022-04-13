#pragma once

/// @file userver/storages/clickhouse/io/columns/uint64_column.hpp
/// @brief UInt64 column support
/// @ingroup userver_clickhouse_types

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

/// @brief Represents ClickHouse UInt64 column
class UInt64Column final : public ClickhouseColumn<UInt64Column> {
 public:
  using cpp_type = std::uint64_t;
  using container_type = std::vector<cpp_type>;

  UInt64Column(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
