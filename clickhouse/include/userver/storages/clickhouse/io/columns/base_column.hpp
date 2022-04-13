#pragma once

/// @file userver/storages/clickhouse/io/columns/base_column.hpp
/// @brief @copybrief storages::clickhouse::io::columns::ClickhouseColumn

#include <userver/storages/clickhouse/io/columns/column_iterator.hpp>
#include <userver/storages/clickhouse/io/columns/column_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

// clang-format off
/// @brief Base class for every typed ClickHouse column.
/// To add new columns one should derive from this class,
/// define types aliases:
/// - `cpp_type` - to what C++ types are column values mapped,
/// - `container_type` (being `std::vector<cpp_type>`)
///
/// and implement 3 methods:
/// - `ctor(ColumnRef)` - column constructor,
/// - `static ColumnRef Serialize(const container_type&)` - constructs a column from C++ container,
/// - `cpp_type ColumnIterator<YourColumnType>::DataHolder::Get()`
///
/// see implementation of any of the existing columns for better understanding.
// clang-format on
template <typename ColumnType>
class ClickhouseColumn {
 public:
  using iterator = ColumnIterator<ColumnType>;

  ClickhouseColumn(ColumnRef column) : column_{std::move(column)} {}

  iterator begin() const { return iterator{column_}; }

  iterator end() const { return iterator::End(column_); }

  size_t Size() const { return GetColumnSize(column_); }

 private:
  ColumnRef column_;
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
