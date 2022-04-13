#pragma once

/// @file userver/storages/clickhouse/io/columns/datetime64_column.hpp
/// @brief DateTime64 columns support
/// @ingroup userver_clickhouse_types

#include <chrono>

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

/// @brief Helper class for instantiating DateTime64 columns
///
/// see
///  - storages::clickhouse::io::columns::DateTime64ColumnMilli
///  - storages::clickhouse::io::columns::DateTime64ColumnMicro
///  - storages::clickhouse::io::columns::DateTime64ColumnNano
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

/// @brief Represents ClickHouse DateTime64(3) column
using DateTime64ColumnMilli = DateTime64Column<3, std::chrono::milliseconds>;

/// @brief Represents ClickHouse DateTime64(6) column
using DateTime64ColumnMicro = DateTime64Column<6, std::chrono::microseconds>;

/// @brief Represents ClickHouse DateTime64(9) column
using DateTime64ColumnNano = DateTime64Column<9, std::chrono::nanoseconds>;

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
