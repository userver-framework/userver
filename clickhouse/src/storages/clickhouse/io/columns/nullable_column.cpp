#include <userver/storages/clickhouse/io/columns/nullable_column.hpp>

#include <storages/clickhouse/io/columns/impl/column_includes.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

NullableColumnMeta ExtractNullableMeta(const ColumnRef& column) {
  auto nullable =
      column->As<clickhouse::impl::clickhouse_cpp::ColumnNullable>();
  if (!nullable) {
    throw std::runtime_error{
        fmt::format("failed to cast column of type '{}' to Nullable",
                    column->Type()->GetName())};
  }

  NullableColumnMeta result;
  result.nulls = nullable->Nulls();
  result.inner = nullable->Nested();
  return result;
}

ColumnRef ConvertMetaToColumn(NullableColumnMeta&& meta) {
  auto nullable = clickhouse::impl::clickhouse_cpp::ColumnNullable(
      std::move(meta.inner), std::move(meta.nulls));
  return std::make_shared<decltype(nullable)>(std::move(nullable));
}

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
