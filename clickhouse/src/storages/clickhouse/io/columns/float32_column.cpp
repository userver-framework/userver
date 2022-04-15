#include <userver/storages/clickhouse/io/columns/float32_column.hpp>

#include <storages/clickhouse/io/columns/impl/numeric_column.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

namespace {
using NativeType = clickhouse::impl::clickhouse_cpp::ColumnFloat32;
}

Float32Column::Float32Column(ColumnRef column)
    : ClickhouseColumn{
          impl::GetTypedColumn<Float32Column, NativeType>(column)} {}

template <>
Float32Column::cpp_type ColumnIterator<Float32Column>::DataHolder::Get() const {
  return impl::NativeGetAt<NativeType>(column_, ind_);
}

ColumnRef Float32Column::Serialize(const container_type& from) {
  return impl::NumericColumn<Float32Column>::Serialize(from);
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
