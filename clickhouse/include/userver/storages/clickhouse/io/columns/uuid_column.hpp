#pragma once

#include <boost/uuid/uuid.hpp>

#include <userver/storages/clickhouse/io/columns/column_includes.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io::columns {

class UuidColumn final : public ClickhouseColumn<UuidColumn> {
 public:
  using cpp_type = boost::uuids::uuid;
  using container_type = std::vector<cpp_type>;

  UuidColumn(ColumnRef column);

  static ColumnRef Serialize(const container_type& from);
};

}  // namespace storages::clickhouse::io::columns

USERVER_NAMESPACE_END
