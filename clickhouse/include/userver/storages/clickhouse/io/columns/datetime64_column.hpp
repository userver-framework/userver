#pragma once

#include <chrono>

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

template <size_t Precision, typename T>
class DateTime64Column;

template <size_t Precision, typename Rep, typename Period,
          template <typename, typename> typename Duration>
class DateTime64Column<Precision, Duration<Rep, Period>>
    : public ClickhouseColumn<
          DateTime64Column<Precision, Duration<Rep, Period>>> {
 public:
  using cpp_type = std::chrono::system_clock::time_point;
  using container_type = std::vector<cpp_type>;

  struct Tag final {
    static constexpr size_t kPrecision = Precision;
    using time_resolution = Duration<Rep, Period>;
  };
  using time_resolution = typename Tag::time_resolution;

  DateTime64Column(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

using DateTime64ColumnMilli = DateTime64Column<3, std::chrono::milliseconds>;
using DateTime64ColumnMicro = DateTime64Column<6, std::chrono::microseconds>;
using DateTime64ColumnNano = DateTime64Column<9, std::chrono::nanoseconds>;

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
