#pragma once

#include <userver/storages/clickhouse/impl/block_wrapper_fwd.hpp>

#include <storages/clickhouse/impl/wrap_clickhouse_cpp.hpp>

#include <clickhouse/block.h>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

class BlockWrapper final {
public:
    // Takes Block by value; all call sites pass an rvalue (std::move or temporary).
    // Using Block instead of Block&& because clickhouse_cpp::Block lacks a move constructor,
    // so Block&& would still copy. The by-value signature makes the copy explicit
    // while allowing move-construction once the library adds it.
    BlockWrapper(clickhouse_cpp::Block block);

    clickhouse_cpp::ColumnRef At(size_t ind) const;

    size_t GetColumnsCount() const;

    size_t GetRowsCount() const;

    void AppendColumn(std::string_view name, const clickhouse_cpp::ColumnRef& column);

    const clickhouse_cpp::Block& GetNative() const;

private:
    clickhouse_cpp::Block native_;
};

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
