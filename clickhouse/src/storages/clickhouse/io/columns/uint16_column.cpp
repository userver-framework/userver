#include <userver/storages/clickhouse/io/columns/uint16_column.hpp>

#include <storages/clickhouse/io/columns/impl/numeric_column.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

namespace {
using NativeType = clickhouse::impl::clickhouse_cpp::ColumnUInt8;
}

UInt16Column::UInt16Column(ColumnRef column)
    : ClickhouseColumn{impl::GetTypedColumn<UInt16Column, NativeType>(column)} {
}

template <>
UInt16Column::cpp_type ColumnIterator<UInt16Column>::DataHolder::Get() const {
  return impl::NativeGetAt<NativeType>(column_, ind_);
}

ColumnRef UInt16Column::Serialize(const container_type& from) {
  return impl::NumericColumn<UInt16Column>::Serialize(from);
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
