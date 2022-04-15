#include <userver/storages/clickhouse/io/columns/float64_column.hpp>

#include <storages/clickhouse/io/columns/impl/numeric_column.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

namespace {
using NativeType = clickhouse::impl::clickhouse_cpp::ColumnFloat64;
}

Float64Column::Float64Column(ColumnRef column)
    : ClickhouseColumn{
          impl::GetTypedColumn<Float64Column, NativeType>(column)} {}

template <>
Float64Column::cpp_type ColumnIterator<Float64Column>::DataHolder::Get() const {
  return impl::NativeGetAt<NativeType>(column_, ind_);
}

ColumnRef Float64Column::Serialize(const container_type& from) {
  return impl::NumericColumn<Float64Column>::Serialize(from);
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
