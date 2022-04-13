#include <userver/storages/clickhouse/io/columns/string_column.hpp>

#include <storages/clickhouse/io/columns/impl/column_includes.hpp>

#include <clickhouse/columns/string.h>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

namespace {
using NativeType = clickhouse::impl::clickhouse_cpp::ColumnString;
}

StringColumn::StringColumn(ColumnRef column)
    : ClickhouseColumn{impl::GetTypedColumn<StringColumn, NativeType>(column)} {
}

template <>
StringColumn::cpp_type ColumnIterator<StringColumn>::DataHolder::Get() const {
  return std::string{impl::NativeGetAt<NativeType>(column_, ind_)};
}

ColumnRef StringColumn::Serialize(const container_type& from) {
  return std::make_shared<clickhouse::impl::clickhouse_cpp::ColumnString>(from);
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
