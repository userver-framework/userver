#pragma once

#include <chrono>

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

template <size_t Precision>
class DateTime64Column : public ClickhouseColumn<DateTime64Column<Precision>> {
 public:
  using cpp_type = std::chrono::system_clock::time_point;
  using container_type = std::vector<cpp_type>;
  static constexpr size_t precision = Precision;

  DateTime64Column(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

using DateTime64ColumnMilli = DateTime64Column<3>;
using DateTime64ColumnMicro = DateTime64Column<6>;
using DateTime64ColumnNano = DateTime64Column<9>;

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
