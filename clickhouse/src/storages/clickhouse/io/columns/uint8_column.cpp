#include <userver/storages/clickhouse/io/columns/uint8_column.hpp>

#include <storages/clickhouse/io/columns/impl/numeric_column.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

namespace {
using NativeType = clickhouse::impl::clickhouse_cpp::ColumnUInt8;
}

UInt8Column::UInt8Column(ColumnRef column)
    : ClickhouseColumn{impl::GetTypedColumn<UInt8Column, NativeType>(column)} {}

template <>
UInt8Column::cpp_type ColumnIterator<UInt8Column>::DataHolder::Get() const {
  return impl::NativeGetAt<NativeType>(column_, ind_);
}

ColumnRef UInt8Column::Serialize(const container_type& from) {
  return impl::NumericColumn<UInt8Column>::Serialize(from);
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
