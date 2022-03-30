#include <userver/storages/clickhouse/io/columns/int32_column.hpp>

#include <storages/clickhouse/io/columns/impl/numeric_column.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

namespace {
using NativeType = clickhouse::impl::clickhouse_cpp::ColumnInt32;
}

Int32Column::Int32Column(ColumnRef column)
    : ClickhouseColumn{impl::GetTypedColumn<Int32Column, NativeType>(column)} {}

template <>
Int32Column::cpp_type BaseIterator<Int32Column>::DataHolder::Get() const {
  // We know the type, see ctor
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  return static_cast<NativeType*>(column_.get())->At(ind_);
}

ColumnRef Int32Column::Serialize(const container_type& from) {
  return impl::NumericColumn<Int32Column>::Serialize(from);
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
