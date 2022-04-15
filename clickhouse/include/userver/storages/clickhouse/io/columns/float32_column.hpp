#pragma once

/// @file userver/storages/clickhouse/io/columns/float32_column.hpp
/// @brief Float32 column support
/// @ingroup userver_clickhouse_types

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

/// @brief Represents Clickhouse Float32 column
class Float32Column final : public ClickhouseColumn<Float32Column> {
 public:
  using cpp_type = float;
  using container_type = std::vector<cpp_type>;

  Float32Column(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
