#pragma once

/// @file userver/storages/clickhouse/io/columns/string_column.hpp
/// @brief String column support
/// @ingroup userver_clickhouse_types

#include <string>

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

/// @brief Represents ClickHouse String column
class StringColumn final : public ClickhouseColumn<StringColumn> {
 public:
  using cpp_type = std::string;
  using container_type = std::vector<cpp_type>;

  StringColumn(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
