#include <userver/utest/utest.hpp>

#include <boost/uuid/uuid.hpp>
#include <vector>

#include <userver/utils/boost_uuid4.hpp>

#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/storages/clickhouse/query.hpp>

#include "utils_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

struct DataWithUuidMismatchedEndianness final {
  std::vector<boost::uuids::uuid> uuids;
};

struct DataWithUuid final {
  std::vector<boost::uuids::uuid> uuids;
};

}  // namespace

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<DataWithUuidMismatchedEndianness> {
  using mapped_type = std::tuple<columns::MismatchedEndiannessUuidColumn>;
};

template <>
struct CppToClickhouse<DataWithUuid> {
  using mapped_type = std::tuple<columns::UuidRfc4122Column>;
};

}  // namespace storages::clickhouse::io

namespace {

template <typename UuidType>
void PerformInsertSelect() {
  ClusterWrapper cluster{};
  cluster->Execute(
      "CREATE TEMPORARY TABLE IF NOT EXISTS tmp_table "
      "(value UUID)");

  const auto uuid = utils::generators::GenerateBoostUuid();

  const UuidType insert_data{{uuid}};
  cluster->Insert("tmp_table", {"value"}, insert_data);

  const auto select_data =
      cluster->Execute("SELECT value FROM tmp_table").As<UuidType>();

  EXPECT_EQ(select_data.uuids.size(), 1);
  EXPECT_EQ(select_data.uuids.front(), insert_data.uuids.front());
}

}  // namespace

UTEST(Uuid, MismatchedEndiannessInsertSelect) {
  PerformInsertSelect<DataWithUuidMismatchedEndianness>();
}

UTEST(Uuid, CorrectEndiannessInsertSelect) {
  PerformInsertSelect<DataWithUuid>();
}

USERVER_NAMESPACE_END
