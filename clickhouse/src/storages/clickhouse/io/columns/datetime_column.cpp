#include <userver/storages/clickhouse/io/columns/datetime_column.hpp>

#include <storages/clickhouse/io/columns/impl/column_includes.hpp>

#include <clickhouse/columns/date.h>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

namespace {
using NativeType = clickhouse::impl::clickhouse_cpp::ColumnDateTime;
}

DateTimeColumn::DateTimeColumn(ColumnRef column)
    : ClickhouseColumn{
          impl::GetTypedColumn<DateTimeColumn, NativeType>(column)} {}

template <>
DateTimeColumn::cpp_type ColumnIterator<DateTimeColumn>::DataHolder::Get()
    const {
  const auto time = impl::NativeGetAt<NativeType>(column_, ind_);
  return std::chrono::system_clock::from_time_t(time);
}

ColumnRef DateTimeColumn::Serialize(const container_type& from) {
  auto column = clickhouse::impl::clickhouse_cpp::ColumnDateTime{};

  for (const auto tp : from) {
    column.Append(std::chrono::system_clock::to_time_t(tp));
  }

  return std::make_shared<decltype(column)>(std::move(column));
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
