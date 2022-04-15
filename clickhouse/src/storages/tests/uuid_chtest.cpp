#include <userver/utest/utest.hpp>

#include <boost/uuid/uuid.hpp>
#include <vector>

#include <userver/utils/boost_uuid4.hpp>

#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/storages/clickhouse/query.hpp>

#include "utils_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

struct DataWithUuid final {
  std::vector<boost::uuids::uuid> uuids;
};

}  // namespace

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<DataWithUuid> {
  using mapped_type = std::tuple<columns::UuidColumn>;
};

}  // namespace storages::clickhouse::io

UTEST(Uuid, InsertSelect) {
  ClusterWrapper cluster{};
  cluster->Execute(
      "CREATE TEMPORARY TABLE IF NOT EXISTS tmp_table "
      "(value UUID)");

  const auto uuid = utils::generators::GenerateBoostUuid();

  const DataWithUuid insert_data{{uuid}};
  cluster->Insert("tmp_table", {"value"}, insert_data);

  const auto select_data =
      cluster->Execute("SELECT value FROM tmp_table").As<DataWithUuid>();
  EXPECT_EQ(select_data.uuids.size(), 1);
  EXPECT_EQ(select_data.uuids.front(), insert_data.uuids.front());
}

USERVER_NAMESPACE_END
