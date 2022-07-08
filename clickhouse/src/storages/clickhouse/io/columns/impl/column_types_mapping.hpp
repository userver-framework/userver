#pragma once

#include <fmt/format.h>

#include <userver/compiler/demangle.hpp>
#include <userver/utils/assert.hpp>

#include <storages/clickhouse/impl/block_wrapper.hpp>
#include <storages/clickhouse/impl/wrap_clickhouse_cpp.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns::impl {

template <typename ColumnType, typename NativeColumnType>
auto GetTypedColumn(
    const clickhouse::impl::clickhouse_cpp::ColumnRef& generic_column_ptr) {
  auto column_ptr = generic_column_ptr->As<NativeColumnType>();
  if (!column_ptr) {
    throw std::runtime_error{
        fmt::format("failed to cast column of type '{}' to '{}'",
                    generic_column_ptr->Type()->GetName(),
                    USERVER_NAMESPACE::compiler::GetTypeName<ColumnType>())};
  }

  return column_ptr;
}

template <typename NativeColumnType>
auto NativeGetAt(const clickhouse::impl::clickhouse_cpp::ColumnRef& column,
                 size_t ind) {
  UASSERT(column->As<NativeColumnType>() != nullptr);
  return static_cast<NativeColumnType*>(column.get())->At(ind);
}

}  // namespace storages::clickhouse::io::columns::impl

USERVER_NAMESPACE_END
