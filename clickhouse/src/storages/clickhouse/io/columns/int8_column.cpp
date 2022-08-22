#include <userver/storages/clickhouse/io/columns/int8_column.hpp>

#include <storages/clickhouse/io/columns/impl/numeric_column.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

namespace {
using NativeType = clickhouse::impl::clickhouse_cpp::ColumnInt8;
}

Int8Column::Int8Column(ColumnRef column)
    : ClickhouseColumn{impl::GetTypedColumn<Int8Column, NativeType>(column)} {}

template <>
Int8Column::cpp_type ColumnIterator<Int8Column>::DataHolder::Get() const {
  return impl::NativeGetAt<NativeType>(column_, ind_);
}

ColumnRef Int8Column::Serialize(const container_type& from) {
  return impl::NumericColumn<Int8Column>::Serialize(from);
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
