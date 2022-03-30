#pragma once

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

class Int64Column final : public ClickhouseColumn<Int64Column> {
 public:
  using cpp_type = std::int64_t;
  using container_type = std::vector<cpp_type>;

  Int64Column(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
