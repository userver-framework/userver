#pragma once

/// @file userver/storages/clickhouse/io/columns/uuid_column.hpp
/// @brief UUID column support
/// @ingroup userver_clickhouse_types

#include <boost/uuid/uuid.hpp>

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

/// @brief Represents ClickHouse UUID column
class UuidColumn final : public ClickhouseColumn<UuidColumn> {
 public:
  using cpp_type = boost::uuids::uuid;
  using container_type = std::vector<cpp_type>;

  UuidColumn(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
