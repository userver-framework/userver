#pragma once

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

class Int32Column final : public ClickhouseColumn<Int32Column> {
 public:
  using cpp_type = std::int32_t;
  using container_type = std::vector<cpp_type>;

  Int32Column(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
