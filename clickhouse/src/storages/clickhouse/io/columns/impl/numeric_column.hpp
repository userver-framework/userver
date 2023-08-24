#pragma once

#include <string>
#include <vector>

#include <storages/clickhouse/impl/wrap_clickhouse_cpp.hpp>
#include <storages/clickhouse/io/columns/impl/column_includes.hpp>
#include <storages/clickhouse/io/columns/impl/column_types_mapping.hpp>

#include <clickhouse/columns/numeric.h>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns::impl {

template <typename ColumnType>
struct NumericColumn final {
  using value_type = typename ColumnType::cpp_type;
  using container_type = typename ColumnType::container_type;

  static clickhouse::impl::clickhouse_cpp::ColumnRef Serialize(
      const container_type& from) {
    return std::make_shared<
        clickhouse::impl::clickhouse_cpp::ColumnVector<value_type>>(from);
  }
};

}  // namespace storages::clickhouse::io::columns::impl

USERVER_NAMESPACE_END
