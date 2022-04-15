#pragma once

/// @file userver/storages/clickhouse/io/columns/float64_column.hpp
/// @brief Float64 column support
/// @ingroup userver_clickhouse_types

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

/// @brief Represents Clickhouse Float64 column
class Float64Column final : public ClickhouseColumn<Float64Column> {
 public:
  using cpp_type = double;
  using container_type = std::vector<cpp_type>;

  Float64Column(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
