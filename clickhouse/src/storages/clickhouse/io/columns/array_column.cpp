#include <userver/storages/clickhouse/io/columns/array_column.hpp>

#include <storages/clickhouse/io/columns/impl/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

namespace {
using ArrayNativeType = clickhouse::impl::clickhouse_cpp::ColumnArray;
using UInt64NativeType = clickhouse::impl::clickhouse_cpp::ColumnUInt64;
}  // namespace

ColumnRef ExtractArrayItem(const ColumnRef& column, size_t ind) {
  auto array_native = column->As<ArrayNativeType>();
  return array_native->GetAsColumn(ind);
}

ColumnRef ConvertMetaToColumn(ArrayColumnMeta&& meta) {
  auto offsets_native =
      impl::GetTypedColumn<UInt64Column, UInt64NativeType>(meta.offsets);
  return std::make_shared<ArrayNativeType>(std::move(meta.data),
                                           std::move(offsets_native));
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
