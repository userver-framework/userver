#include <userver/storages/clickhouse/io/columns/datetime64_column.hpp>

#include <storages/clickhouse/io/columns/impl/column_includes.hpp>

#include <clickhouse/columns/date.h>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

namespace {

using NativeType = clickhouse::impl::clickhouse_cpp::ColumnDateTime64;

std::chrono::system_clock::time_point DoGetDate(const ColumnRef& column,
                                                size_t ind) {
  // We know the type, see GetDatetimeColumn
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  const auto nanos = static_cast<NativeType*>(column.get())->At(ind);

  using clock = std::chrono::system_clock;
  return clock::time_point{std::chrono::duration_cast<clock::duration>(
      std::chrono::nanoseconds{nanos})};
}

template <typename DateColumnType>
ColumnRef DoSerializeDate(
    const std::vector<std::chrono::system_clock::time_point>& from) {
  auto column = clickhouse::impl::clickhouse_cpp::ColumnDateTime64(
      DateColumnType::precision);
  for (const auto tp : from) {
    column.Append(std::chrono::duration_cast<std::chrono::nanoseconds>(
                      tp.time_since_epoch())
                      .count());
  }

  return std::make_shared<decltype(column)>(std::move(column));
}

template <typename ColumnType>
ColumnRef GetDatetimeColumn(const ColumnRef& column) {
  return impl::GetTypedColumn<ColumnType, NativeType>(column);
}

}  // namespace

template <>
DateTime64ColumnMilli::DateTime64Column(ColumnRef column)
    : ClickhouseColumn{GetDatetimeColumn<DateTime64ColumnMilli>(column)} {}

template <>
DateTime64ColumnMicro::DateTime64Column(ColumnRef column)
    : ClickhouseColumn{GetDatetimeColumn<DateTime64ColumnMilli>(column)} {}

template <>
DateTime64ColumnNano::DateTime64Column(ColumnRef column)
    : ClickhouseColumn{GetDatetimeColumn<DateTime64ColumnMilli>(column)} {}

template <>
DateTime64ColumnMilli::cpp_type
BaseIterator<DateTime64ColumnMilli>::DataHolder::Get() const {
  return DoGetDate(column_, ind_);
}

template <>
DateTime64ColumnMicro::cpp_type
BaseIterator<DateTime64ColumnMicro>::DataHolder::Get() const {
  return DoGetDate(column_, ind_);
}

template <>
DateTime64ColumnNano::cpp_type
BaseIterator<DateTime64ColumnNano>::DataHolder::Get() const {
  return DoGetDate(column_, ind_);
}

template <>
ColumnRef DateTime64ColumnMilli::Serialize(const container_type& from) {
  return DoSerializeDate<DateTime64ColumnMilli>(from);
}

template <>
ColumnRef DateTime64ColumnMicro::Serialize(const container_type& from) {
  return DoSerializeDate<DateTime64ColumnMicro>(from);
}

template <>
ColumnRef DateTime64ColumnNano::Serialize(const container_type& from) {
  return DoSerializeDate<DateTime64ColumnNano>(from);
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
