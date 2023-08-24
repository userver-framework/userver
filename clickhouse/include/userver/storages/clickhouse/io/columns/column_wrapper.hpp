#pragma once

#include <memory>
#include <string_view>

#include <userver/storages/clickhouse/impl/block_wrapper_fwd.hpp>

namespace clickhouse {
class Column;
}

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

using ColumnRef = std::shared_ptr<::clickhouse::Column>;

ColumnRef GetWrappedColumn(clickhouse::impl::BlockWrapper& block, size_t ind);

void AppendWrappedColumn(clickhouse::impl::BlockWrapper& block,
                         ColumnRef&& column, std::string_view name, size_t ind);

size_t GetColumnSize(const ColumnRef& column);

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
