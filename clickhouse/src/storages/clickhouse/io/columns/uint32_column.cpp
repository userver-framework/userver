#include <userver/storages/clickhouse/io/columns/uint32_column.hpp>

#include <storages/clickhouse/io/columns/impl/numeric_column.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

namespace {
using NativeType = clickhouse::impl::clickhouse_cpp::ColumnUInt32;
}

UInt32Column::UInt32Column(ColumnRef column)
    : ClickhouseColumn{impl::GetTypedColumn<UInt32Column, NativeType>(column)} {
}

template <>
UInt32Column::cpp_type ColumnIterator<UInt32Column>::DataHolder::Get() const {
  return impl::NativeGetAt<NativeType>(column_, ind_);
}

ColumnRef UInt32Column::Serialize(const container_type& from) {
  return impl::NumericColumn<UInt32Column>::Serialize(from);
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
