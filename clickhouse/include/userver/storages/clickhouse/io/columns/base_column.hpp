#pragma once

#include <userver/storages/clickhouse/io/columns/column_iterator.hpp>
#include <userver/storages/clickhouse/io/columns/column_wrapper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

template <typename ColumnType>
class ClickhouseColumn {
 public:
  using iterator = BaseIterator<ColumnType>;

  ClickhouseColumn(ColumnRef column) : column_{std::move(column)} {}

  iterator begin() const { return iterator{column_}; }

  iterator end() const { return iterator::End(column_); }

  size_t Size() const { return GetColumnSize(column_); }

 private:
  ColumnRef column_;
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
