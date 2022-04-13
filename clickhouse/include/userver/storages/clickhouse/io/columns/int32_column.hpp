#pragma once

/// @file userver/storages/clickhouse/io/columns/int32_column.hpp
/// @brief Int32 column support
/// @ingroup userver_clickhouse_types

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

/// @brief Represents Clickhouse Int32 column
class Int32Column final : public ClickhouseColumn<Int32Column> {
 public:
  using cpp_type = std::int32_t;
  using container_type = std::vector<cpp_type>;

  Int32Column(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
