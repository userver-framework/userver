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
UInt32Column::cpp_type BaseIterator<UInt32Column>::DataHolder::Get() const {
  // We know the type, see ctor
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  return static_cast<NativeType*>(column_.get())->At(ind_);
}

ColumnRef UInt32Column::Serialize(const container_type& from) {
  return impl::NumericColumn<UInt32Column>::Serialize(from);
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
