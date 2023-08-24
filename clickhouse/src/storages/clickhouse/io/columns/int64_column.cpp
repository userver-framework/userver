#include <userver/storages/clickhouse/io/columns/int64_column.hpp>

#include <storages/clickhouse/io/columns/impl/numeric_column.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

namespace {
using NativeType = clickhouse::impl::clickhouse_cpp::ColumnInt64;
}

Int64Column::Int64Column(ColumnRef column)
    : ClickhouseColumn{impl::GetTypedColumn<Int64Column, NativeType>(column)} {}

template <>
Int64Column::cpp_type ColumnIterator<Int64Column>::DataHolder::Get() const {
  return impl::NativeGetAt<NativeType>(column_, ind_);
}

ColumnRef Int64Column::Serialize(const container_type& from) {
  return impl::NumericColumn<Int64Column>::Serialize(from);
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
