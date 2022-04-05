#pragma once

#include <chrono>

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

class DateTimeColumn final : public ClickhouseColumn<DateTimeColumn> {
 public:
  using cpp_type = std::chrono::system_clock::time_point;
  using container_type = std::vector<cpp_type>;

  DateTimeColumn(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
