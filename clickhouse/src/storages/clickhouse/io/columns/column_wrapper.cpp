#include <userver/storages/clickhouse/io/columns/column_wrapper.hpp>

#include <userver/utils/assert.hpp>

#include <storages/clickhouse/impl/block_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

ColumnRef GetWrappedColumn(clickhouse::impl::BlockWrapper& block, size_t ind) {
  UINVARIANT(ind < block.GetColumnsCount(), "Shouldn't happen");

  return block.At(ind);
}

void AppendWrappedColumn(clickhouse::impl::BlockWrapper& block,
                         ColumnRef&& column, std::string_view name,
                         size_t ind) {
  UINVARIANT(block.GetColumnsCount() == ind, "Shouldn't happen");

  block.AppendColumn(name, column);
}

size_t GetColumnSize(const ColumnRef& column) { return column->Size(); }

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
