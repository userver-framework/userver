#pragma once

/// @file userver/storages/clickhouse/io/columns/uint16_column.hpp
/// @brief UInt16 column support
/// @ingroup userver_clickhouse_types

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

/// @brief Represents Clickhouse UInt16 column
class UInt16Column final : public ClickhouseColumn<UInt16Column> {
 public:
  using cpp_type = uint16_t;
  using container_type = std::vector<cpp_type>;

  UInt16Column(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
